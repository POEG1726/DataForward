idf_component_register(SRCS "user_uart.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES driver "TCPServer"
                    )
