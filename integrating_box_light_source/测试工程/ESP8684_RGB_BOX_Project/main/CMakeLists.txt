# 积分盒四色光源板固件
# hello_world_main.c 是模板残留，不参与编译（它定义了第二个 app_main）
idf_component_register(SRCS "main.c"
                            "bsp_i2c.c"
                            "drv_mcp4728.c"
                            "drv_ads1110.c"
                            "app_light.c"
                            "app_console.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES driver console nvs_flash)
