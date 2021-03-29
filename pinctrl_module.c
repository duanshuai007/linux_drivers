
static const struct of_device_id owl_pinctrl_of_match[] = { 
    { 
            .compatible = "actions,s900-pinctrl", 
    }, 
    {

    }, 
}; 
MODULE_DEVICE_TABLE(of, owl_pinctrl_of_match); 



static const struct pinctrl_pin_desc s900_pins[] = { 
        PINCTRL_PIN(6, "A7"), 
        PINCTRL_PIN(15, "B6"), 
         PINCTRL_PIN(24, "C5"), 
        PINCTRL_PIN(37, "D8"), 
         PINCTRL_PIN(60, "G1"), 
        PINCTRL_PIN(61, "G2"), 
};


//Pin Multiplexing
static const char * const uart5_groups[] = { 
        "uart5_0_grp", "uart5_1_grp", "uart5_2_grp" 
};
static const struct owl_pmx_func s900_functions[] = { 
        { 
                .name = "uart5", 
                .groups = uart5_groups, 
                 .num_groups = ARRAY_SIZE(uart5_groups), 
         }, 
};

static struct pinmux_ops s900_pmxops = { 
         .get_functions_count = s900_get_functions_count, 
        .get_function_name = s900_get_fname, 
        .get_function_groups = s900_get_groups, 
        .set_mux = s900_set_mux, 
        .strict = true, 
};




static const struct of_device_id owl_pinctrl_of_match[] = { 
        { 
                 .compatible = "actions,s900-pinctrl", 
                .data = &s900_desc, 
        }, 
        { 
        }, 
};

static int __init owl_pinctrl_probe(struct platform_device *pdev) 
{  
        ...
         desc = of_device_get_match_data(dev); 
        if (desc == NULL) { 
                  dev_err(dev, "device get match data failed\n"); 
                 return -ENODEV; 
        }
         pctl = pinctrl_register(desc, dev, NULL); 
        ... 

        return 0;           
} 

static void __exit owl_pinctrl_remove(struct platform_device *pdev) 
{ 

} 

static struct platform_driver owl_pinctrl_platform_driver = { 
        .probe          = owl_pinctrl_probe, 
        .remove         = owl_pinctrl_remove, 
        .driver         = { 
                .name   = "owl_pinctrl", 
                .of_match_table = owl_pinctrl_of_match, 
        }, 
}; 
module_platform_driver(owl_pinctrl_platform_driver); 

MODULE_ALIAS("platform driver: owl_pinctrl"); 
MODULE_DESCRIPTION("pinctrl driver for owl seria soc(s900, etc.)"); 
MODULE_AUTHOR("wowo<wowo@wowotech.net>"); 
MODULE_LICENSE("GPL v2");