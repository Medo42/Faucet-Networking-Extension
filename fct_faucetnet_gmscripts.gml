#define read_delimited_string
var bytesleft, result;
bytesleft = fct_buffer_bytes_left(argument0);
result = _fnet_hidden_read_delimited_string(argument0, argument1);
if(bytesleft == fct_buffer_bytes_left(argument0))
	if(argument1 != "")
		return -1;

return result;

#define read_cstring
var bytesleft, result;
bytesleft = fct_buffer_bytes_left(argument0);
result = _fnet_hidden_read_cstring(argument0);
if(bytesleft == fct_buffer_bytes_left(argument0))
	return -1;

return result;