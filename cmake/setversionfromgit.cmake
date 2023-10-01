find_package(Git)

function(setversionfromgit)
    if(Git_FOUND)
        message("Found Git executable ${GIT_EXECUTABLE}, version ${GIT_VERSION_STRING}")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags  --match "v*"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
            RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            string(REGEX MATCHALL "-.*$|[0-9]+" PARTIAL_VERSION_LIST ${GIT_DESCRIBE_VERSION})
            
            list(GET PARTIAL_VERSION_LIST 0 MAJOR)
            list(GET PARTIAL_VERSION_LIST 1 MINOR)

            set(PROJECT_VERSION_MAJOR ${MAJOR} PARENT_SCOPE)
            set(PROJECT_VERSION_MINOR ${MINOR} PARENT_SCOPE)
            set(CMAKE_PROJECT_VERSION ${MAJOR}.${MINOR}.0 PARENT_SCOPE)
            
    else()
        message("Could not find git executable")
    endif()



    
endfunction(setversionfromgit)
