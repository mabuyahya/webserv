<?php
header("Content-Type: application/json; charset=utf-8");

$body = file_get_contents("php://input");
$payload = array(
    "method" => getenv("REQUEST_METHOD"),
    "query" => getenv("QUERY_STRING"),
    "content_type" => getenv("CONTENT_TYPE"),
    "content_length" => getenv("CONTENT_LENGTH"),
    "body" => $body,
    "script_name" => getenv("SCRIPT_NAME"),
    "server_protocol" => getenv("SERVER_PROTOCOL"),
    "php_sapi" => php_sapi_name()
);

echo json_encode($payload, JSON_PRETTY_PRINT);
?>
