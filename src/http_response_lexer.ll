%option noyywrap nounput noinput nodefault
%option stack
%option reentrant
%option extra-type="http_response_context *"
%{
#include <boost/system/error_code.hpp>
#include <network/protocols/http/response_header.hpp>
#include <network/protocols/http/response_status.hpp>
#include <cstdlib>

using namespace ::network::protocols::http;
%}

%x IN_status_code IN_status_message IN_header_key IN_header_value
%x IN_content_type IN_content_length IN_transfer_coding

%%

"HTTP/1.0 "         BEGIN(IN_status_code); yyextra->protocol_version = HTTP_1_0;
"HTTP/1.1 "         BEGIN(IN_status_code); yyextra->protocol_version = HTTP_1_1;
.|\n                return HTTP_RESPONSE_MALFORMED;

<IN_status_code>{
    [0-9]{3}        BEGIN(IN_status_message); yyextra->status_code = std::atoi(yytext);
    .|\n            return HTTP_RESPONSE_MALFORMED;
}

<IN_status_message>{
    .+              ;
    \n              BEGIN(IN_header_key);
}

<IN_header_key>{
    (?i:Content-Type:) {
                    BEGIN(IN_content_type);
    }
    (?i:Content-Length:) {
                    BEGIN(IN_content_length);
    }
    (?i:Transfer-Coding:) {
                    BEGIN(IN_transfer_coding);
    }
    [^ \t\r\n:]+    yyextra->accept_header_key(yytext);
    :               BEGIN(IN_header_value);
    [ \t\r]         ;
    \n              return HTTP_RESPONSE_OK;
}

<IN_header_value>{
    [^ \t\r\n]+     yyextra->accept_header_value(yytext);
    [ \t\r]         ;
    \n              BEGIN(IN_header_key);
}

<IN_content_type>{
    [^ \t\r\n]+     yyextra->accept_content_type(yytext);
    [ \t\r]         ;
    \n              BEGIN(IN_header_key);
}

<IN_content_length>{
    [^ \t\r\n]+     yyextra->content_length = atoi(yytext);
    [ \t\r]         ;
    \n              BEGIN(IN_header_key);
}

<IN_transfer_coding>{
    [^ \t\r\n]+     yyextra->accept_transfer_coding(yytext);
    [ \t\r]         ;
    \n              BEGIN(IN_header_key);
}

%%

namespace network { namespace protocols { namespace http {

boost::system::error_code parse_response_header(const char *buf, size_t size, http_response_context& out_response_header)
{
    using boost::system::make_error_code;

    yyscan_t scanner;
    yylex_init_extra(&out_response_header, &scanner);
    YY_BUFFER_STATE buffer_state = yy_scan_bytes(buf, size, scanner);

    yyset_debug(1, scanner);
    int ret = yylex(scanner);

    yy_delete_buffer(buffer_state, scanner);
    yylex_destroy(scanner);

    return make_error_code(static_cast<response_status_t>(ret));
}

}}} // namespace network { namespace protocols { namespace http {
