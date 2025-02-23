set(requiers 
    "user_uart"
    "LED"
    "TCPServer"
    nvs_flash
    )

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${requiers}
                    )
