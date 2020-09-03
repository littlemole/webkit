
# contrat

messages are sent via webkit user messages.

WebKitUserMessage name is request or response
params is always a signle gvariant holding a JSON encoded string.

## request JSON

{
    'request' : "<uid>",
    'method' : 'methodName',
    "parameters" : [ ... ]
}

## response JSON

{
    'response' : "<uid>",
    "result" : value,
    "exception" : "error-string"
}



