#include <linux/pinctrl/pinctrl.h>

const struct pinctrl_pin_desc foo_pins[] = {
      PINCTRL_PIN(0, "A8"),
      PINCTRL_PIN(1, "B8"),
      PINCTRL_PIN(2, "C8"),
      ...
      PINCTRL_PIN(61, "F1"),
      PINCTRL_PIN(62, "G1"),
      PINCTRL_PIN(63, "H1"),
};

static struct pinctrl_desc foo_desc = {
    .name = "foo",
    .pins = foo_pins,
    .npins = ARRAY_SIZE(foo_pins),
    .maxpin = 63,
    .owner = THIS_MODULE,
};

int __init foo_probe(void)
{
    struct pinctrl_dev *pctl;

    pctl = pinctrl_register(&foo_desc, <PARENT>, NULL);
    if (IS_ERR(pctl))
        pr_err("could not register foo pin driver\n");
}

#include <linux/pinctrl/pinctrl.h>
struct foo_group {
    const char *name;
    const unsigned int *pins;
    const unsigned num_pins;
};

static const unsigned int spi0_pins[] = { 0, 8, 16, 24 };
static const unsigned int i2c0_pins[] = { 24, 25 };

static const struct foo_group foo_groups[] = {
    {
        .name = "spi0_grp",
        .pins = spi0_pins,
        .num_pins = ARRAY_SIZE(spi0_pins),
    },
    {
        .name = "i2c0_grp",
        .pins = i2c0_pins,
        .num_pins = ARRAY_SIZE(i2c0_pins),
    },
};


static int foo_get_groups_count(struct pinctrl_dev *pctldev)
{
    return ARRAY_SIZE(foo_groups);
}

static const char *foo_get_group_name(struct pinctrl_dev *pctldev,
                       unsigned selector)
{
    return foo_groups[selector].name;
}

static int foo_get_group_pins(struct pinctrl_dev *pctldev, unsigned selector,
                   unsigned ** const pins,
                   unsigned * const num_pins)
{
    *pins = (unsigned *) foo_groups[selector].pins;
    *num_pins = foo_groups[selector].num_pins;
    return 0;
}

static struct pinctrl_ops foo_pctrl_ops = {
    .get_groups_count = foo_get_groups_count,
    .get_group_name = foo_get_group_name,
    .get_group_pins = foo_get_group_pins,
};

static struct pinctrl_desc foo_desc = {
       ...
       .pctlops = &foo_pctrl_ops,
};

//设置一个引脚为上拉
#include <linux/pinctrl/consumer.h>

ret = pin_config_set("foo-dev", "FOO_GPIO_PIN", PLATFORM_X_PULL_UP);

//

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include "platform_x_pindefs.h"

static int foo_pin_config_get(struct pinctrl_dev *pctldev,
            unsigned offset,
            unsigned long *config)
{
    struct my_conftype conf;

    ... Find setting for pin @ offset ...

    *config = (unsigned long) conf;
}

static int foo_pin_config_set(struct pinctrl_dev *pctldev,
            unsigned offset,
            unsigned long config)
{
    struct my_conftype *conf = (struct my_conftype *) config;

    switch (conf) {
        case PLATFORM_X_PULL_UP:
        ...
        }
    }
}

static int foo_pin_config_group_get (struct pinctrl_dev *pctldev,
            unsigned selector,
            unsigned long *config)
{
    ...
}

static int foo_pin_config_group_set (struct pinctrl_dev *pctldev,
            unsigned selector,
            unsigned long config)
{
    ...
}

static struct pinconf_ops foo_pconf_ops = {
    .pin_config_get = foo_pin_config_get,
    .pin_config_set = foo_pin_config_set,
    .pin_config_group_get = foo_pin_config_group_get,
    .pin_config_group_set = foo_pin_config_group_set,
};
