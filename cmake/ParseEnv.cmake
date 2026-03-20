# ParseEnv.cmake - Parse .env file and set CMake variables

function(parse_env_file env_file)
    if(NOT EXISTS "${env_file}")
        message(FATAL_ERROR "Environment file not found: ${env_file}")
    endif()

    file(READ "${env_file}" env_content)
    
    # Split by newlines
    string(REGEX MATCHALL "[^\n]+" lines "${env_content}")
    
    foreach(line ${lines})
        # Skip comments and empty lines
        if(NOT line MATCHES "^#" AND NOT line STREQUAL "")
            # Parse KEY=VALUE
            string(REGEX MATCH "^[^=]+" key "${line}")
            string(REGEX MATCH "=.*" value "${line}")
            
            if(key AND value)
                # Remove the leading = from value
                string(SUBSTRING "${value}" 1 -1 value)
                # Trim whitespace
                string(STRIP "${value}" value)
                string(STRIP "${key}" key)
                
                # Set as CMake variable in parent scope
                set(${key} "${value}" PARENT_SCOPE)
                message(STATUS "ENV: ${key}=${value}")
            endif()
        endif()
    endforeach()
endfunction()

function(generate_config_header env_file output_header)
    if(NOT EXISTS "${env_file}")
        message(FATAL_ERROR "Environment file not found: ${env_file}")
    endif()

    file(READ "${env_file}" env_content)
    
    # Split by newlines
    string(REGEX MATCHALL "[^\n]+" lines "${env_content}")
    
    # Parse all environment variables into local scope
    foreach(line ${lines})
        # Skip comments and empty lines
        if(NOT line MATCHES "^#" AND NOT line STREQUAL "")
            # Parse KEY=VALUE
            string(REGEX MATCH "^[^=]+" key "${line}")
            string(REGEX MATCH "=.*" value "${line}")
            
            if(key AND value)
                # Remove the leading = from value
                string(SUBSTRING "${value}" 1 -1 value)
                # Trim whitespace
                string(STRIP "${value}" value)
                string(STRIP "${key}" key)
                
                # Remove surrounding quotes if present
                if(value MATCHES "^\".*\"$")
                    string(LENGTH "${value}" value_len)
                    math(EXPR end_index "${value_len} - 2")
                    string(SUBSTRING "${value}" 1 ${end_index} value)
                endif()
                
                # Set as local CMake variable
                set(${key} "${value}")
                message(STATUS "ENV: ${key}=${value}")
            endif()
        endif()
    endforeach()
    
    # Generate header file with all variables available in local scope
    file(WRITE "${output_header}" "#ifndef APP_CONFIG_H\n")
    file(APPEND "${output_header}" "#define APP_CONFIG_H\n\n")
    file(APPEND "${output_header}" "#include <QString>\n\n")
    file(APPEND "${output_header}" "namespace AppConfig {\n")
    file(APPEND "${output_header}" "    const QString SPOTIFY_CLIENT_ID = \"${SPOTIFY_CLIENT_ID}\";\n")
    file(APPEND "${output_header}" "    const QString SPOTIFY_CLIENT_SECRET = \"${SPOTIFY_CLIENT_SECRET}\";\n")
    file(APPEND "${output_header}" "    constexpr int POLL_INTERVAL_MS = ${APP_POLL_INTERVAL_MS};\n")
    file(APPEND "${output_header}" "    constexpr int REDIRECT_PORT = ${APP_REDIRECT_PORT};\n")
    file(APPEND "${output_header}" "}\n\n")
    file(APPEND "${output_header}" "#endif // APP_CONFIG_H\n")
    
    message(STATUS "Generated config header: ${output_header}")
endfunction()

