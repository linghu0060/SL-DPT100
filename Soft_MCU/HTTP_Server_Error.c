/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network:Service
 * Copyright (c) 2004-2014 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HTTP_Server_Error.c
 * Purpose: HTTP Server Friendly Error Message Definitions
 * Rev.:    V6.00
 *----------------------------------------------------------------------------*/
//! [code_HTTP_Server_Error]
#include "rl_net_lib.h"
 
// Keep HTTP Error page size small
HTTP_ERROR http_error = {
  // HTTP Error page header
  "<head><title>Server Error</title></head>"
  "<body>",
 
  // HTTP Error page footer
  "<hr><br>"
  "<i>Embedded WEB Server, 2015<br>"
  "<a href=www.github.com>www.github.com</a>"
  " - Embedded Development Tools</i>"
  "</body>",
 
  // HTTP Error 401 - Unauthorized access
  "<h2>Error 401 - Unauthorized Access</h2>"
  "You are not authorized to access this server.",
 
  // HTTP Error 403 - Forbidden
  "<h2>Error 403 - Forbidden</h2>"
  "You don't have permission to access this resource.",
 
  // HTTP Error 404 - Not Found
  "<h2>Error 404 - Not Found</h2>"
  "The requested URL was not found on this server.",
 
  // HTTP Error 501 - Not Implemented
  "<h2>Error 501 - Not Implemented</h2>"
  "The requested Method is not supported."
};
//! [code_HTTP_Server_Error]
