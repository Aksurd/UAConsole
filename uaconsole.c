/*
 * ============================================================================
 * UAConsole - Universal OPC UA Server Console Browser
 * ============================================================================
 * 
 * Professional command-line tool for OPC UA server inspection and diagnostics.
 * Lightweight, cross-platform solution for industrial automation engineers.
 * 
 * Developer:    Alexander Dikunov
 * Contact:      wxid_ic7ytyv3mlh522 (WeChat)
 * Email:        aksurd@gmail.com
 * 
 * Version:      1.0.0
 * Release:      December 2025
 * 
 * Acknowledgments: DeepSeek AI development assistance
 * 
 * ============================================================================
 *                               DISCLAIMER
 * ============================================================================
 * 
 * This software is provided for TESTING AND DIAGNOSTIC PURPOSES ONLY.
 * NOT intended for use in safety-critical systems or production environments
 * without thorough validation by qualified personnel.
 * 
 * The developer assumes NO LIABILITY for any damages, data loss, or system
 * failures resulting from the use of this software.
 * 
 * Always verify server compatibility and test in isolated environments first.
 * 
 * ============================================================================
 * MIT License
 * ============================================================================
 * 
 * Copyright (c) 2025 Alexander Dikunov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ============================================================================
 *                               BUILD INSTRUCTIONS
 * ============================================================================
 * 
 * 1. Install dependencies:
 *    ----------------------------------------------
 *    # Ubuntu/Debian
 *    sudo apt-get install libopen62541-dev
 *    
 *    # From source (if package not available)
 *    git clone https://github.com/open62541/open62541.git
 *    cd open62541
 *    mkdir build && cd build
 *    cmake -DBUILD_SHARED_LIBS=ON ..
 *    make -j$(nproc)
 *    sudo make install
 *    sudo ldconfig
 * 
 * 2. Compile this program:
 *    ----------------------------------------------
 *    gcc -o uaconsole browse_opc_server.c -lopen62541 -lm
 *    
 *    # Or with optimizations
 *    gcc -O2 -o uaconsole uaconsole.c -lopen62541 -lm
 *    
 *    # Debug build with symbols
 *    gcc -g -O0 -o uaconsole uaconsole.c -lopen62541 -lm
 * 
 * 3. Run:
 *    ----------------------------------------------
 *    ./uaconsole opc.tcp://10.0.0.128:4840
 *    ./uaconsole -h                    # Show help
 *    ./uaconsole -v opc.tcp://...     # Verbose mode
 * 
 * ============================================================================
 */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========== FUNCTION PROTOTYPES ==========
void print_help(const char* program_name);

// Function for recursive node traversal and value reading
static void browseAndReadNode(UA_Client *client, UA_NodeId nodeId, int depth, int verbose) {
    
    // Get node attributes
    UA_NodeClass nodeClass;
    UA_QualifiedName browseName;
    
    if (UA_Client_readNodeClassAttribute(client, nodeId, &nodeClass) != UA_STATUSCODE_GOOD ||
        UA_Client_readBrowseNameAttribute(client, nodeId, &browseName) != UA_STATUSCODE_GOOD) {
        return;
    }
    
    // Create indentation for hierarchy visualization
    char indent[64] = {0};
    for (int i = 0; i < depth && i < 60; i++) {
        indent[i] = ' ';
        indent[i+1] = ' ';
    }
    
    // Display node information
    printf("%s%.*s", indent, (int)browseName.name.length, browseName.name.data);
    
    // Format NodeId for display
    char nodeIdStr[256];
    snprintf(nodeIdStr, sizeof(nodeIdStr), " [ns=%u;", nodeId.namespaceIndex);
    
    if (nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        char temp[64];
        snprintf(temp, sizeof(temp), "i=%u]", nodeId.identifier.numeric);
        strncat(nodeIdStr, temp, sizeof(nodeIdStr) - strlen(nodeIdStr) - 1);
    } else if (nodeId.identifierType == UA_NODEIDTYPE_STRING) {
        char temp[256];
        snprintf(temp, sizeof(temp), "s=%.*s]", 
                (int)nodeId.identifier.string.length,
                nodeId.identifier.string.data);
        strncat(nodeIdStr, temp, sizeof(nodeIdStr) - strlen(nodeIdStr) - 1);
    }
    
    printf(" %s", nodeIdStr);
    
    // Determine node type
    switch (nodeClass) {
        case UA_NODECLASS_OBJECT:
            printf(" (Object)\n");
            break;
        case UA_NODECLASS_VARIABLE:
            printf(" (Variable)");
            // Try to read variable value
            UA_Variant value;
            UA_Variant_init(&value);
            UA_StatusCode readStatus = UA_Client_readValueAttribute(client, nodeId, &value);
            
            if (readStatus == UA_STATUSCODE_GOOD && !UA_Variant_isEmpty(&value)) {
                printf(" = ");
                
                // Display value based on type
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
                    printf("%s", *(UA_Boolean*)value.data ? "true" : "false");
                } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16])) {
                    printf("%u", *(UA_UInt16*)value.data);
                } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32])) {
                    printf("%u", *(UA_UInt32*)value.data);
                } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_FLOAT])) {
                    printf("%.2f", *(UA_Float*)value.data);
                } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
                    UA_DateTimeStruct dts = UA_DateTime_toStruct(*(UA_DateTime*)value.data);
                    printf("%04u-%02u-%02u %02u:%02u:%02u", 
                           dts.year, dts.month, dts.day, 
                           dts.hour, dts.min, dts.sec);
                } else {
                    // For other types, display type name
                    printf("[%s]", value.type->typeName);
                }
            } else {
                printf(" [Read error: 0x%08X]", readStatus);
            }
            printf("\n");
            UA_Variant_clear(&value);
            break;
        case UA_NODECLASS_METHOD:
            printf(" (Method)\n");
            break;
        case UA_NODECLASS_OBJECTTYPE:
            printf(" (ObjectType)\n");
            break;
        case UA_NODECLASS_VARIABLETYPE:
            printf(" (VariableType)\n");
            break;
        case UA_NODECLASS_REFERENCETYPE:
            printf(" (ReferenceType)\n");
            break;
        case UA_NODECLASS_DATATYPE:
            printf(" (DataType)\n");
            break;
        case UA_NODECLASS_VIEW:
            printf(" (View)\n");
            break;
        default:
            printf(" (Unknown)\n");
    }
    
    UA_QualifiedName_clear(&browseName);
    
    // Recursive traversal of child nodes (only for objects)
    if (nodeClass == UA_NODECLASS_OBJECT || nodeClass == UA_NODECLASS_VIEW) {
        UA_BrowseRequest bReq;
        UA_BrowseRequest_init(&bReq);
        bReq.nodesToBrowse = UA_BrowseDescription_new();
        bReq.nodesToBrowseSize = 1;
        bReq.nodesToBrowse[0].nodeId = nodeId;
        bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
        
        UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
        
        if (bResp.resultsSize > 0 && bResp.results[0].referencesSize > 0) {
            if (verbose && depth == 0) {
                printf("  Found %zu references to browse\n", bResp.results[0].referencesSize);
            }
            for (size_t i = 0; i < bResp.results[0].referencesSize; i++) {
                if (bResp.results[0].references[i].isForward) {
                    browseAndReadNode(client, bResp.results[0].references[i].nodeId.nodeId, depth + 1, verbose);
                }
            }
        }
        
        UA_BrowseResponse_clear(&bResp);
        UA_BrowseRequest_clear(&bReq);
    }
}

// ========== HELP FUNCTION ==========
void print_help(const char* program_name) {
    printf("UAConsole - Universal OPC UA Server Console Browser\n");
    printf("=====================================================\n\n");
    printf("Usage: %s [OPTIONS] [SERVER_URL]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help           Show this help message\n");
    printf("  -v, --verbose        Enable verbose output\n");
    printf("  -t, --timeout N      Set connection timeout in ms (default: 5000)\n\n");
    
    printf("Examples:\n");
    printf("  %s opc.tcp://10.0.0.128:4840\n", program_name);
    printf("  %s -v opc.tcp://opcua-esp32:4840\n", program_name);
    printf("  %s -t 10000 opc.tcp://10.0.0.128:4840\n\n", program_name);
    
    printf("Contact:\n");
    printf("  WeChat: wxid_ic7ytyv3mlh522\n");
    printf("  Email:  aksurd@yandex.ru\n\n");
    
    printf("Safety Warning 安全警告:\n");
    printf("  • For testing/diagnostic use only 仅用于测试/诊断\n");
    printf("  • Not for production without validation 未经验证不得用于生产\n");
    printf("  • Test in isolated environment first 先在隔离环境测试\n");
    printf("  • Author assumes no liability 作者不承担任何责任\n\n");
    
    printf("Build & Installation 构建与安装:\n");
    printf("  # Install library 安装库\n");
    printf("  sudo apt-get install libopen62541-dev\n\n");
    printf("  # Compile 编译\n");
    printf("  gcc -o uaconsole browse_opc_server.c -lopen62541 -lm\n\n");
    printf("  # Run 运行\n");
    printf("  ./uaconsole opc.tcp://10.0.0.128:4840\n\n");
    
    printf("Note: Server URL is REQUIRED for browsing.\n");
    printf("      Run without arguments to see this help.\n");
}

int main(int argc, char* argv[]) {
    // ========== SHOW HELP BY DEFAULT ==========
    if (argc == 1) {
        print_help(argv[0]);
        return 0;
    }
    
    // ========== COMMAND LINE PARSING ==========
    
    // Default values
    char* server_url = "opc.tcp://10.0.0.128:4840";
    int verbose = 0;
    int timeout_ms = 5000;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
            if (i + 1 < argc) {
                timeout_ms = atoi(argv[++i]);
                if (timeout_ms <= 0) {
                    printf("Error: Timeout must be positive\n");
                    return 1;
                }
            } else {
                printf("Error: Missing value for timeout\n");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            printf("Unknown option: %s\n", argv[i]);
            printf("Use %s -h for help\n", argv[0]);
            return 1;
        } else {
            // Assume it's the server URL
            server_url = argv[i];
        }
    }
    
    // ========== CLIENT CONFIGURATION ==========
    
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    config->timeout = timeout_ms;
    
    // ========== CONNECTION ==========
    
    printf("=============================================\n");
    printf("   UAConsole - OPC UA Server Browser\n");
    printf("=============================================\n\n");
    
    if (verbose) {
        printf("Verbose mode enabled\n");
        printf("Connection timeout: %d ms\n", timeout_ms);
        printf("Connecting to %s...\n", server_url);
    } else {
        printf("Connecting to %s...\n", server_url);
    }
    
    UA_StatusCode retval = UA_Client_connect(client, server_url);
    
    if(retval != UA_STATUSCODE_GOOD) {
        printf("Connection failed: %s (0x%08X)\n", 
               UA_StatusCode_name(retval), retval);
        UA_Client_delete(client);
        return 1;
    }
    
    printf("Connected successfully!\n\n");
    
    if (verbose) {
        printf("=== CONNECTION DETAILS ===\n");
        UA_DateTimeStruct connectTime = UA_DateTime_toStruct(UA_DateTime_now());
        printf("Connection time: %04u-%02u-%02u %02u:%02u:%02u\n", 
               connectTime.year, connectTime.month, connectTime.day,
               connectTime.hour, connectTime.min, connectTime.sec);
        printf("Timeout configured: %d ms\n", timeout_ms);
        printf("\n");
    }
    
    // ========== SERVER BROWSING ==========
    
    printf("=== RECURSIVE BROWSING OF OBJECTS FOLDER ===\n");
    
    if (verbose) {
        printf("Starting from ObjectsFolder (ns=0;i=85)\n");
        printf("Depth-first traversal...\n\n");
    }
    
    UA_NodeId objectsFolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    browseAndReadNode(client, objectsFolder, 0, verbose);
    
    // ========== DISCONNECTION AND CLEANUP ==========
    
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    
    printf("\n=== BROWSING COMPLETED ===\n");
    printf("Server URL: %s\n", server_url);
    printf("Disconnected from server\n");
    
    return 0;
}
