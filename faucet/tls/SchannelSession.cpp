#include "SchannelSession.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <windows.h>
#include <wincrypt.h>
#include <security.h>
#include <schannel.h>

#include <cstdio>
#include <cstring>

/*
 * Older MinGW headers lack some of the newer definitions. All of these are
 * plain constants / struct layouts documented by Microsoft, so we can just
 * provide them ourselves where needed.
 */
#ifndef SCH_USE_STRONG_CRYPTO
#define SCH_USE_STRONG_CRYPTO 0x00400000
#endif
#ifndef SCH_CRED_MANUAL_CRED_VALIDATION
#define SCH_CRED_MANUAL_CRED_VALIDATION 0x00000008
#endif
#ifndef SP_PROT_TLS1_2_CLIENT
#define SP_PROT_TLS1_2_CLIENT 0x00000800
#endif
#ifndef SECBUFFER_EXTRA
#define SECBUFFER_EXTRA 5
#endif
#ifndef SECPKG_ATTR_REMOTE_CERT_CONTEXT
#define SECPKG_ATTR_REMOTE_CERT_CONTEXT 0x53
#endif

namespace {

/**
 * Same layout as SCH_CREDENTIALS (credentials version 5) from schannel.h,
 * which is the only way to get TLS 1.3 out of SChannel. Declared here under
 * our own name because the real one is only available in recent headers, and
 * only after defining SCHANNEL_USE_BLACKLISTS and pulling in extra headers.
 *
 * We never pass any TLS_PARAMETERS, so that member is left as an opaque pointer.
 */
struct SchCredentialsV5 {
	DWORD dwVersion;
	DWORD dwCredFormat;
	DWORD cCreds;
	PCCERT_CONTEXT *paCred;
	HCERTSTORE hRootStore;
	DWORD cMappers;
	struct _HMAPPER **aphMappers;
	DWORD dwSessionLifespan;
	DWORD dwFlags;
	DWORD cTlsParameters;
	void *pTlsParameters;
};
const DWORD SCH_CREDENTIALS_V5 = 5;

/*
 * - We validate the server certificate ourselves once the handshake is done,
 *   see verifyServerCertificate(). SChannel could do this on its own, but
 *   doing it explicitly keeps the behaviour visible and the same everywhere
 *   (Wine's SChannel for example does not validate anything by itself).
 * - Never send a client certificate on our own.
 * - Don't allow known-weak algorithms.
 */
const DWORD CREDENTIAL_FLAGS = SCH_CRED_MANUAL_CRED_VALIDATION
		| SCH_CRED_NO_DEFAULT_CREDS | SCH_USE_STRONG_CRYPTO;

const DWORD BASE_REQUEST_FLAGS = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT
		| ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

std::string describeStatus(SECURITY_STATUS status) {
	char code[32];
	snprintf(code, sizeof(code), "0x%08lX", static_cast<unsigned long> (status));

	char text[512];
	DWORD length = FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
			static_cast<DWORD> (status), 0, text, sizeof(text), NULL);
	while (length > 0 && (text[length - 1] == '\r' || text[length - 1] == '\n'
			|| text[length - 1] == ' ')) {
		length--;
	}
	if (length == 0) {
		return std::string("error code ") + code;
	}
	return std::string(text, length) + " (" + code + ")";
}

/**
 * Plain words for the usual reasons a certificate gets rejected. The system
 * messages for these are long-winded and depend on the Windows language.
 */
std::string describeCertificateError(DWORD error) {
	const char *reason;
	switch (static_cast<HRESULT> (error)) {
	case CERT_E_UNTRUSTEDROOT:
	case CERT_E_CHAINING:
	case CERT_E_UNTRUSTEDCA:
		reason = "it was not issued by a trusted certificate authority";
		break;
	case CERT_E_CN_NO_MATCH:
		reason = "it was not issued for this host name";
		break;
	case CERT_E_EXPIRED:
	case CERT_E_VALIDITYPERIODNESTING:
		reason = "it has expired or is not valid yet";
		break;
	case CERT_E_REVOKED:
		reason = "it has been revoked";
		break;
	case CERT_E_WRONG_USAGE:
		reason = "it is not meant for identifying servers";
		break;
	case TRUST_E_CERT_SIGNATURE:
		reason = "its signature is not valid";
		break;
	case TRUST_E_FAIL:
		reason = "its chain of trust could not be established";
		break;
	default:
		return describeStatus(static_cast<SECURITY_STATUS> (error));
	}
	char code[32];
	snprintf(code, sizeof(code), " (0x%08lX)", static_cast<unsigned long> (error));
	return std::string(reason) + code;
}

void setBuffer(SecBuffer &buffer, unsigned long type, void *data, size_t size) {
	buffer.BufferType = type;
	buffer.pvBuffer = data;
	buffer.cbBuffer = static_cast<unsigned long> (size);
}

}

struct SchannelSession::Impl {
	enum State {
		HANDSHAKING, ESTABLISHED, SHUT_DOWN, FAILED
	};

	std::string hostname;
	State state;
	std::string errorMessage;

	CredHandle credentials;
	bool haveCredentials;
	CtxtHandle context;
	bool haveContext;
	DWORD requestFlags;
	SecPkgContext_StreamSizes sizes;

	/**
	 * Set when the next handshake step has to be performed even though
	 * there is no new input from the peer.
	 */
	bool stepWithoutInput;
	bool peerClosed;

	/** Ciphertext received from the peer which has not been processed yet */
	std::vector<uint8_t> rawIn;
	/** Ciphertext waiting to be sent to the peer */
	std::vector<uint8_t> rawOut;
	/** Decrypted application data */
	std::vector<uint8_t> plainIn;

	/** The server certificate that passed validation, in encoded form */
	std::vector<uint8_t> verifiedCertificate;

	Impl(const std::string &hostname) :
			hostname(hostname), state(HANDSHAKING), errorMessage(), credentials(),
			haveCredentials(false), context(), haveContext(false),
			requestFlags(BASE_REQUEST_FLAGS), sizes(), stepWithoutInput(false),
			peerClosed(false), rawIn(), rawOut(), plainIn(), verifiedCertificate() {
	}

	~Impl() {
		if (haveContext) {
			DeleteSecurityContext(&context);
		}
		if (haveCredentials) {
			FreeCredentialsHandle(&credentials);
		}
	}

	void fail(const std::string &message) {
		// Keep the first error, it is usually the most meaningful one
		if (state != FAILED) {
			state = FAILED;
			errorMessage = message;
		}
	}

	void fail(const char *what, SECURITY_STATUS status) {
		fail(std::string(what) + describeStatus(status));
	}

	bool acquireCredentials() {
		char packageName[] = UNISP_NAME_A;
		TimeStamp expiry;
		SECURITY_STATUS status;

		/*
		 * First try the current credentials format. It leaves the choice of
		 * protocol versions entirely to the operating system, which includes
		 * TLS 1.3 where the system supports it. This format is understood
		 * by Windows 10 version 1809 and later.
		 */
		SchCredentialsV5 current;
		memset(&current, 0, sizeof(current));
		current.dwVersion = SCH_CREDENTIALS_V5;
		current.dwFlags = CREDENTIAL_FLAGS;
		status = AcquireCredentialsHandleA(NULL, packageName, SECPKG_CRED_OUTBOUND,
				NULL, &current, NULL, NULL, &credentials, &expiry);
		if (status == SEC_E_OK) {
			haveCredentials = true;
			return true;
		}

		/*
		 * Older systems only know the legacy format. There we have to ask
		 * for TLS 1.2 explicitly, because on Windows 7 / 8 the system default
		 * for clients does not include it.
		 */
		SCHANNEL_CRED legacy;
		memset(&legacy, 0, sizeof(legacy));
		legacy.dwVersion = SCHANNEL_CRED_VERSION;
		legacy.dwFlags = CREDENTIAL_FLAGS;
		legacy.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
		status = AcquireCredentialsHandleA(NULL, packageName, SECPKG_CRED_OUTBOUND,
				NULL, &legacy, NULL, NULL, &credentials, &expiry);
		if (status == SEC_E_OK) {
			haveCredentials = true;
			return true;
		}

		fail("TLS is not available: ", status);
		return false;
	}

	/**
	 * Drop the processed part of rawIn. SChannel tells us how many bytes at
	 * the end of the input it has not looked at yet.
	 */
	void keepUnprocessed(const SecBuffer &extra) {
		if (extra.BufferType == SECBUFFER_EXTRA && extra.cbBuffer > 0
				&& extra.cbBuffer <= rawIn.size()) {
			rawIn.erase(rawIn.begin(), rawIn.end() - extra.cbBuffer);
		} else {
			rawIn.clear();
		}
	}

	/**
	 * Check that the certificate presented by the server chains up to an
	 * authority trusted by Windows, is currently valid, may be used for
	 * identifying servers and was issued for the host we wanted to talk to.
	 *
	 * This has to succeed before any application data is sent or handed out.
	 *
	 * Revocation checking is deliberately not requested: It makes Windows fetch
	 * CRLs / OCSP responses over the network right here, which would block the
	 * caller (i.e. the game) for an unpredictable time.
	 */
	bool verifyServerCertificate() {
		PCCERT_CONTEXT certificate = NULL;
		SECURITY_STATUS status = QueryContextAttributes(&context,
				SECPKG_ATTR_REMOTE_CERT_CONTEXT, &certificate);
		if (status != SEC_E_OK || certificate == NULL) {
			fail("The server did not present a certificate: ", status);
			return false;
		}

		/*
		 * Handshakes can happen again on an established connection. No need
		 * to go through all of this again if the certificate is the same.
		 */
		const std::vector<uint8_t> encoded(certificate->pbCertEncoded,
				certificate->pbCertEncoded + certificate->cbCertEncoded);
		if (!verifiedCertificate.empty() && encoded == verifiedCertificate) {
			CertFreeCertificateContext(certificate);
			return true;
		}

		char serverAuth[] = szOID_PKIX_KP_SERVER_AUTH;
		char serverGatedCrypto[] = szOID_SERVER_GATED_CRYPTO;
		char sgcNetscape[] = szOID_SGC_NETSCAPE;
		LPSTR usages[] = { serverAuth, serverGatedCrypto, sgcNetscape };
		CERT_CHAIN_PARA chainPara;
		memset(&chainPara, 0, sizeof(chainPara));
		chainPara.cbSize = sizeof(chainPara);
		chainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
		chainPara.RequestedUsage.Usage.cUsageIdentifier = 3;
		chainPara.RequestedUsage.Usage.rgpszUsageIdentifier = usages;

		// The store attached to the certificate holds the intermediate
		// certificates sent by the server.
		PCCERT_CHAIN_CONTEXT chain = NULL;
		if (!CertGetCertificateChain(NULL, certificate, NULL, certificate->hCertStore,
				&chainPara, 0, NULL, &chain)) {
			DWORD error = GetLastError();
			CertFreeCertificateContext(certificate);
			fail("The server certificate could not be checked: "
					+ describeStatus(static_cast<SECURITY_STATUS> (error)));
			return false;
		}

		std::vector<wchar_t> wideHostname(hostname.size() + 1, 0);
		MultiByteToWideChar(CP_ACP, 0, hostname.c_str(), -1, &wideHostname[0],
				static_cast<int> (wideHostname.size()));

		SSL_EXTRA_CERT_CHAIN_POLICY_PARA sslPara;
		memset(&sslPara, 0, sizeof(sslPara));
		sslPara.cbSize = sizeof(sslPara);
		sslPara.dwAuthType = AUTHTYPE_SERVER;
		sslPara.fdwChecks = 0;
		sslPara.pwszServerName = &wideHostname[0];

		CERT_CHAIN_POLICY_PARA policyPara;
		memset(&policyPara, 0, sizeof(policyPara));
		policyPara.cbSize = sizeof(policyPara);
		policyPara.pvExtraPolicyPara = &sslPara;

		CERT_CHAIN_POLICY_STATUS policyStatus;
		memset(&policyStatus, 0, sizeof(policyStatus));
		policyStatus.cbSize = sizeof(policyStatus);

		DWORD error;
		if (CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, chain, &policyPara,
				&policyStatus)) {
			error = policyStatus.dwError;
		} else {
			error = GetLastError();
			if (error == 0) {
				error = static_cast<DWORD> (E_FAIL);
			}
		}

		/*
		 * The policy check above is supposed to cover everything, but not
		 * every implementation is equally thorough (Wine's lets incomplete
		 * chains pass). So also insist that building the chain itself did
		 * not run into any problem that matters to us.
		 */
		if (error == 0) {
			const DWORD irrelevant = CERT_TRUST_IS_NOT_TIME_NESTED
					| CERT_TRUST_REVOCATION_STATUS_UNKNOWN | CERT_TRUST_IS_OFFLINE_REVOCATION;
			const DWORD problems = chain->TrustStatus.dwErrorStatus & ~irrelevant;
			if (problems & CERT_TRUST_IS_NOT_SIGNATURE_VALID) {
				error = static_cast<DWORD> (TRUST_E_CERT_SIGNATURE);
			} else if (problems & CERT_TRUST_IS_UNTRUSTED_ROOT) {
				error = static_cast<DWORD> (CERT_E_UNTRUSTEDROOT);
			} else if (problems & CERT_TRUST_IS_PARTIAL_CHAIN) {
				error = static_cast<DWORD> (CERT_E_CHAINING);
			} else if (problems & CERT_TRUST_IS_NOT_TIME_VALID) {
				error = static_cast<DWORD> (CERT_E_EXPIRED);
			} else if (problems & CERT_TRUST_IS_REVOKED) {
				error = static_cast<DWORD> (CERT_E_REVOKED);
			} else if (problems & CERT_TRUST_IS_NOT_VALID_FOR_USAGE) {
				error = static_cast<DWORD> (CERT_E_WRONG_USAGE);
			} else if (problems != 0) {
				error = static_cast<DWORD> (TRUST_E_FAIL);
			}
		}
		CertFreeCertificateChain(chain);
		CertFreeCertificateContext(certificate);

		if (error != 0) {
			fail("The server certificate was rejected: " + describeCertificateError(error));
			return false;
		}
		verifiedCertificate = encoded;
		return true;
	}

	/**
	 * Advance the handshake as far as possible with the input we have.
	 */
	void handshakeStep() {
		while (state == HANDSHAKING) {
			const bool firstCall = !haveContext;
			if (!firstCall && rawIn.empty() && !stepWithoutInput) {
				// Waiting for the peer
				return;
			}
			stepWithoutInput = false;

			uint8_t nothing = 0;
			SecBuffer inBuffers[2];
			setBuffer(inBuffers[0], SECBUFFER_TOKEN,
					rawIn.empty() ? &nothing : &rawIn[0], rawIn.size());
			setBuffer(inBuffers[1], SECBUFFER_EMPTY, NULL, 0);
			SecBufferDesc inDesc = { SECBUFFER_VERSION, 2, inBuffers };

			SecBuffer outBuffers[1];
			setBuffer(outBuffers[0], SECBUFFER_TOKEN, NULL, 0);
			SecBufferDesc outDesc = { SECBUFFER_VERSION, 1, outBuffers };

			DWORD attributes = 0;
			TimeStamp expiry;
			SECURITY_STATUS status = InitializeSecurityContextA(&credentials,
					firstCall ? NULL : &context,
					const_cast<SEC_CHAR *> (hostname.c_str()), requestFlags, 0, 0,
					firstCall ? NULL : &inDesc, 0, firstCall ? &context : NULL,
					&outDesc, &attributes, &expiry);

			if (outBuffers[0].pvBuffer != NULL) {
				const uint8_t *token = static_cast<const uint8_t *> (outBuffers[0].pvBuffer);
				rawOut.insert(rawOut.end(), token, token + outBuffers[0].cbBuffer);
				FreeContextBuffer(outBuffers[0].pvBuffer);
			}

			if (status == SEC_E_INCOMPLETE_MESSAGE) {
				// Only part of a handshake message has arrived so far.
				return;
			}

			if (status == SEC_I_INCOMPLETE_CREDENTIALS
					&& !(requestFlags & ISC_REQ_USE_SUPPLIED_CREDS)) {
				/*
				 * The server asks for a client certificate. We have none, so
				 * tell SChannel to carry on without one and repeat the step
				 * with the same input.
				 */
				requestFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
				stepWithoutInput = true;
				continue;
			}

			if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED) {
				fail("TLS handshake failed: ", status);
				return;
			}

			haveContext = true;
			if (!firstCall) {
				keepUnprocessed(inBuffers[1]);
			}

			if (status == SEC_E_OK) {
				status = QueryContextAttributes(&context, SECPKG_ATTR_STREAM_SIZES,
						&sizes);
				if (status != SEC_E_OK) {
					fail("TLS handshake failed: ", status);
					return;
				}
				if (!verifyServerCertificate()) {
					return;
				}
				state = ESTABLISHED;
			}
		}
	}

	/**
	 * Decrypt as many complete TLS records from rawIn as possible.
	 */
	void decryptRecords() {
		while (state == ESTABLISHED && !peerClosed && !rawIn.empty()) {
			SecBuffer buffers[4];
			setBuffer(buffers[0], SECBUFFER_DATA, &rawIn[0], rawIn.size());
			setBuffer(buffers[1], SECBUFFER_EMPTY, NULL, 0);
			setBuffer(buffers[2], SECBUFFER_EMPTY, NULL, 0);
			setBuffer(buffers[3], SECBUFFER_EMPTY, NULL, 0);
			SecBufferDesc desc = { SECBUFFER_VERSION, 4, buffers };

			SECURITY_STATUS status = DecryptMessage(&context, &desc, 0, NULL);

			if (status == SEC_E_INCOMPLETE_MESSAGE) {
				// Only part of a record has arrived so far.
				return;
			}

			if (status != SEC_E_OK && status != SEC_I_RENEGOTIATE
					&& status != SEC_I_CONTEXT_EXPIRED) {
				fail("TLS decryption failed: ", status);
				return;
			}

			/*
			 * The record was decrypted in place, the buffer descriptors now
			 * point to the parts of rawIn holding the plaintext and the
			 * input that was not looked at yet.
			 */
			const size_t sizeBefore = rawIn.size();
			SecBuffer extra;
			setBuffer(extra, SECBUFFER_EMPTY, NULL, 0);
			bool gotData = false;
			for (int i = 0; i < 4; i++) {
				if (buffers[i].BufferType == SECBUFFER_DATA && buffers[i].cbBuffer > 0) {
					const uint8_t *data = static_cast<const uint8_t *> (buffers[i].pvBuffer);
					plainIn.insert(plainIn.end(), data, data + buffers[i].cbBuffer);
					gotData = true;
				} else if (buffers[i].BufferType == SECBUFFER_EXTRA) {
					extra = buffers[i];
				}
			}
			keepUnprocessed(extra);

			if (status == SEC_I_CONTEXT_EXPIRED) {
				// The peer has sent a close notification.
				peerClosed = true;
				return;
			}

			if (status == SEC_I_RENEGOTIATE) {
				/*
				 * The peer has sent a handshake message. With TLS 1.2 and
				 * below that is a request to renegotiate. With TLS 1.3 this
				 * is how SChannel reports post-handshake messages like new
				 * session tickets, which arrive right after the handshake of
				 * pretty much every connection. Either way, whatever input
				 * was left over has to go through the handshake function.
				 */
				state = HANDSHAKING;
				stepWithoutInput = true;
				handshakeStep();
			} else if (!gotData && rawIn.size() == sizeBefore) {
				// Should not happen, but never spin in place.
				fail("TLS decryption failed: no progress");
				return;
			}
		}
	}

	void process() {
		if (state == HANDSHAKING) {
			handshakeStep();
		}
		if (state == ESTABLISHED) {
			decryptRecords();
		}
	}
};

SchannelSession::SchannelSession(const std::string &hostname) :
		impl_(new Impl(hostname)) {
	if (impl_->acquireCredentials()) {
		// Produces the first handshake message
		impl_->handshakeStep();
	}
}

SchannelSession::~SchannelSession() {
}

void SchannelSession::feed(const uint8_t *data, size_t size) {
	if (impl_->state == Impl::FAILED || impl_->state == Impl::SHUT_DOWN) {
		return;
	}
	impl_->rawIn.insert(impl_->rawIn.end(), data, data + size);
	impl_->process();
}

std::vector<uint8_t> &SchannelSession::outgoing() {
	return impl_->rawOut;
}

std::vector<uint8_t> &SchannelSession::incoming() {
	return impl_->plainIn;
}

bool SchannelSession::encrypt(const uint8_t *data, size_t size) {
	if (impl_->state != Impl::ESTABLISHED) {
		return false;
	}

	const SecPkgContext_StreamSizes &sizes = impl_->sizes;
	std::vector<uint8_t> record;
	while (size > 0) {
		const size_t chunk = size < sizes.cbMaximumMessage ? size : sizes.cbMaximumMessage;
		record.resize(sizes.cbHeader + chunk + sizes.cbTrailer);
		memcpy(&record[sizes.cbHeader], data, chunk);

		SecBuffer buffers[4];
		setBuffer(buffers[0], SECBUFFER_STREAM_HEADER, &record[0], sizes.cbHeader);
		setBuffer(buffers[1], SECBUFFER_DATA, &record[sizes.cbHeader], chunk);
		setBuffer(buffers[2], SECBUFFER_STREAM_TRAILER,
				&record[sizes.cbHeader + chunk], sizes.cbTrailer);
		setBuffer(buffers[3], SECBUFFER_EMPTY, NULL, 0);
		SecBufferDesc desc = { SECBUFFER_VERSION, 4, buffers };

		SECURITY_STATUS status = EncryptMessage(&impl_->context, 0, &desc, 0);
		if (status != SEC_E_OK) {
			impl_->fail("TLS encryption failed: ", status);
			return false;
		}

		// The trailer may have ended up shorter than the space we reserved for it
		const size_t recordSize = buffers[0].cbBuffer + buffers[1].cbBuffer
				+ buffers[2].cbBuffer;
		impl_->rawOut.insert(impl_->rawOut.end(), record.begin(),
				record.begin() + recordSize);

		data += chunk;
		size -= chunk;
	}
	return true;
}

void SchannelSession::shutdown() {
	if (impl_->state != Impl::ESTABLISHED) {
		return;
	}
	impl_->state = Impl::SHUT_DOWN;

	DWORD controlToken = SCHANNEL_SHUTDOWN;
	SecBuffer controlBuffers[1];
	setBuffer(controlBuffers[0], SECBUFFER_TOKEN, &controlToken, sizeof(controlToken));
	SecBufferDesc controlDesc = { SECBUFFER_VERSION, 1, controlBuffers };
	if (ApplyControlToken(&impl_->context, &controlDesc) != SEC_E_OK) {
		return;
	}

	// The handshake function now produces the close notification for us
	SecBuffer outBuffers[1];
	setBuffer(outBuffers[0], SECBUFFER_TOKEN, NULL, 0);
	SecBufferDesc outDesc = { SECBUFFER_VERSION, 1, outBuffers };
	DWORD attributes = 0;
	TimeStamp expiry;
	InitializeSecurityContextA(&impl_->credentials, &impl_->context,
			const_cast<SEC_CHAR *> (impl_->hostname.c_str()), impl_->requestFlags,
			0, 0, NULL, 0, NULL, &outDesc, &attributes, &expiry);
	if (outBuffers[0].pvBuffer != NULL) {
		const uint8_t *token = static_cast<const uint8_t *> (outBuffers[0].pvBuffer);
		impl_->rawOut.insert(impl_->rawOut.end(), token, token + outBuffers[0].cbBuffer);
		FreeContextBuffer(outBuffers[0].pvBuffer);
	}
}

bool SchannelSession::isHandshaking() const {
	return impl_->state == Impl::HANDSHAKING;
}

bool SchannelSession::isEstablished() const {
	return impl_->state == Impl::ESTABLISHED;
}

bool SchannelSession::peerClosed() const {
	return impl_->peerClosed;
}

bool SchannelSession::hasError() const {
	return impl_->state == Impl::FAILED;
}

const std::string &SchannelSession::getErrorMessage() const {
	return impl_->errorMessage;
}
