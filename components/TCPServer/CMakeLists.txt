idf_component_register(SRCS "TCPServer.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES driver esp_wifi json "user_uart" 
                    )
