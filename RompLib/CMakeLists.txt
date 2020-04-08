find_package(glog REQUIRED)

file(GLOB SOURCES src/*.cpp)
file(GLOB PAPI_SOURCES papi/*.c)

add_library(romp SHARED ${SOURCES})

find_library(SYMTABAPI "libsymtabAPI.so")
          
target_include_directories(romp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	                        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/papi)

target_link_libraries(romp glog ${SYMTABAPI})
install(TARGETS romp 
        LIBRARY DESTINATION lib)
