#define read_binary_string
var result;
result = min(buffer_bytes_left(argument0), argument1) * " ";
_fnet_hidden_read_binary_string(argument0, result, "");
return result;

#define read_delimited_binary_string
var length, result;
length = _fnet_hidden_bytes_before_delimiter(argument0, argument1);
if (length < 0)
	return -1;
result = length * " ";
_fnet_hidden_read_binary_string(argument0, result, argument1);
return result;

#define read_delimited_string
var bytesleft, result;
bytesleft = buffer_bytes_left(argument0);
result = _fnet_hidden_read_delimited_string(argument0, argument1);
if(bytesleft == buffer_bytes_left(argument0))
	if(argument1 != "")
		return -1;
return result;

#define read_cstring
var bytesleft, result;
bytesleft = buffer_bytes_left(argument0);
result = _fnet_hidden_read_cstring(argument0);
if(bytesleft == buffer_bytes_left(argument0))
	return -1;
return result;