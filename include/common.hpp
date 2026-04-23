#ifndef COMMON_HPP
#define COMMON_HPP

enum HttpResponseCode {
	OK = 200,
	BadRequest = 400,
	NotFound = 404,
	InternalServerError = 500
};

enum HTTPMethod {
	GET,
	POST,
	DELETE
};

#endif
