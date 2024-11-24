#include <linux/module.h>
#include <linux/idr.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/regmap.h>

#include <linux/i2c.h>

#include "tca9535_mux.h"

#define MODULE_DEVICE_NAME	"tca9535-mux"

#define TCA9535_REGISTER_INPUT_0			0x00
#define TCA9535_REGISTER_INPUT_1			0x01
#define TCA9535_REGISTER_OUTPUT_0			0x02 	// Output Port 0, default 1111_1111
#define TCA9535_REGISTER_OUTPUT_1			0x03	// Output Port 1, default 1111_1111
#define TCA9535_REGISTER_POLARITY_0			0x04	// Polarity Inversion Port 0, default 0000_0000, not inverted
#define TCA9535_REGISTER_POLARITY_1			0x05	// Polarity Inversion Port 1, default 0000_0000, not inverted
#define TCA9535_REGISTER_CONFIGURATION_0	0x06	// Configuration Port 0, default 1111_1111, input
#define TCA9535_REGISTER_CONFIGURATION_1	0x07	// Configuration Port 0, default 1111_1111, input

static DEFINE_IDR(mux_id);
static DEFINE_MUTEX(mux_mutex);

static bool tca9535_readable_register(struct device *dev, unsigned int reg)
{
	return true;
}

static bool tca9535_writeable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case TCA9535_REGISTER_INPUT_0:
	case TCA9535_REGISTER_INPUT_1:
		return false;
	default:
		return true;
	}
}

static bool tca9535_volatile_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case TCA9535_REGISTER_INPUT_0:
	case TCA9535_REGISTER_INPUT_1:
		return true;
	default:
		return false;
	}
}

static const struct regmap_config tca9535_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.use_single_read = true,
	.use_single_write = true,
	.readable_reg = tca9535_readable_register,
	.writeable_reg = tca9535_writeable_register,
	.volatile_reg = tca9535_volatile_register,

	.disable_locking = true,
	.cache_type = REGCACHE_RBTREE,
	.max_register = 0x07,
};

ssize_t address_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct tca9535_device_info *di = dev_get_drvdata(dev);
	return sysfs_emit(buf, "%d\n", di->iic_address);
}
static DEVICE_ATTR_RO(address);


ssize_t input_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct tca9535_device_info *di = dev_get_drvdata(dev);
	unsigned int inp_0, inp_1;
	int ret;
	bool has_error = false;

	mutex_lock(&di->lock);
	ret = regmap_read(di->regmap, TCA9535_REGISTER_INPUT_0, &inp_0);
	if (ret) {
		dev_warn(di->dev, "Cannot read Input register 0: %d\n", ret);
		has_error = true;
	}

	ret = regmap_read(di->regmap, TCA9535_REGISTER_INPUT_1, &inp_1);
	if (ret) {
		dev_warn(di->dev, "Cannot read Input register 1: %d\n", ret);
		has_error = true;
	}
	mutex_unlock(&di->lock);
	
	inp_0 = (inp_1 << 8) | inp_0;
	if (has_error) {
		return sysfs_emit(buf, "error\n");
	} else {
		return sysfs_emit(buf, "%u\n", inp_0);
	}
}
static DEVICE_ATTR_RO(input);


ssize_t out_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct tca9535_device_info *di = dev_get_drvdata(dev);
	unsigned int out_0, out_1;
	int ret;
	bool has_error = false;

	mutex_lock(&di->lock);
	ret = regmap_read(di->regmap, TCA9535_REGISTER_OUTPUT_0, &out_0);
	if (ret) {
		dev_warn(di->dev, "Cannot read output register 0: %d\n", ret);
		has_error = true;
	}

	ret = regmap_read(di->regmap, TCA9535_REGISTER_OUTPUT_1, &out_1);
	if (ret) {
		dev_warn(di->dev, "Cannot read output register 1: %d\n", ret);
		has_error = true;
	}
	mutex_unlock(&di->lock);

	out_0 = (out_1 << 8) | out_0;
	if (has_error) {
		return sysfs_emit(buf, "error\n");
	} else {
		return sysfs_emit(buf, "%u\n", out_0);
	}
}

ssize_t out_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct tca9535_device_info *di = dev_get_drvdata(dev);
	unsigned int tmp, out_0, out_1;
	int ret;
	bool has_error;

	ret = kstrtouint(buf, 10, &tmp);
	if (ret)
		return ret;
	
	if (tmp > 65535) {
		dev_warn(di->dev, "Out value %u to big\n", tmp);
		return count;
	}

	out_0 = tmp & 0xFF;
	out_1 = tmp >> 8;
	mutex_lock(&di->lock);
	ret = regmap_write(di->regmap, TCA9535_REGISTER_OUTPUT_0, out_0);
	if (ret) {
		dev_warn(di->dev, "Cannot write Output 0: %d\n", ret);
		has_error = true;
	}

	ret = regmap_write(di->regmap, TCA9535_REGISTER_OUTPUT_1, out_1);
	if (ret) {
		dev_warn(di->dev, "Cannot write Output 1: %d\n", ret);
		has_error = true;
	}

	mutex_unlock(&di->lock);
	return count;
}
static DEVICE_ATTR(out, 00660, out_show, out_store);


ssize_t config_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct tca9535_device_info *di = dev_get_drvdata(dev);
	unsigned int cfg_0, cfg_1;
	int ret;
	bool has_error = false;

	mutex_lock(&di->lock);
	ret = regmap_read(di->regmap, TCA9535_REGISTER_CONFIGURATION_0, &cfg_0);
	if (ret) {
		dev_warn(di->dev, "Cannot read Config register 0: %d\n", ret);
		has_error = true;
	}

	ret = regmap_read(di->regmap, TCA9535_REGISTER_CONFIGURATION_1, &cfg_1);
	if (ret) {
		dev_warn(di->dev, "Cannot read Config register 1: %d\n", ret);
		has_error = true;
	}
	mutex_unlock(&di->lock);
	
	cfg_0 = (cfg_1 << 8) | cfg_0;
	if (has_error) {
		return sysfs_emit(buf, "error\n");
	} else {
		return sysfs_emit(buf, "%u\n", cfg_0);
	}
}

ssize_t config_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct tca9535_device_info *di = dev_get_drvdata(dev);
	unsigned int tmp, cfg_0, cfg_1;
	int ret;

	ret = kstrtouint(buf, 10, &tmp);
	if (ret)
		return ret;
	
	if (tmp > 65535) {
		dev_warn(di->dev, "Out value %u to big\n", tmp);
		return count;
	}

	cfg_0 = tmp & 0xFF;
	cfg_1 = tmp >> 8;
	mutex_lock(&di->lock);
	ret = regmap_write(di->regmap, TCA9535_REGISTER_CONFIGURATION_0, cfg_0);
	if (ret) {
		dev_warn(di->dev, "Cannot write Configuration 0: %d\n", ret);
	}

	ret = regmap_write(di->regmap, TCA9535_REGISTER_CONFIGURATION_1, cfg_1);
	if (ret) {
		dev_warn(di->dev, "Cannot write Configuration 1: %d\n", ret);
	}

	mutex_unlock(&di->lock);
	return count;
}
static DEVICE_ATTR(config, 00660, config_show, config_store);


ssize_t polarity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct tca9535_device_info *di = dev_get_drvdata(dev);
	unsigned int pol_0, pol_1;
	int ret;
	bool has_error = false;

	mutex_lock(&di->lock);
	ret = regmap_read(di->regmap, TCA9535_REGISTER_POLARITY_0, &pol_0);
	if (ret) {
		dev_warn(di->dev, "Cannot read Polarity register 0: %d\n", ret);
		has_error = true;
	}

	ret = regmap_read(di->regmap, TCA9535_REGISTER_POLARITY_1, &pol_1);
	if (ret) {
		dev_warn(di->dev, "Cannot read Polarity register 1: %d\n", ret);
		has_error = true;
	}
	mutex_unlock(&di->lock);
	
	pol_0 = (pol_1 << 8) | pol_0;
	if (has_error) {
		return sysfs_emit(buf, "error\n");
	} else {
		return sysfs_emit(buf, "%u\n", pol_0);
	}
}

ssize_t polarity_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct tca9535_device_info *di = dev_get_drvdata(dev);
	unsigned int tmp, pol_0, pol_1;
	int ret;

	ret = kstrtouint(buf, 10, &tmp);
	if (ret)
		return ret;
	
	if (tmp > 65535) {
		dev_warn(di->dev, "Out value %u to big\n", tmp);
		return count;
	}

	pol_0 = tmp & 0xFF;
	pol_1 = tmp >> 8;
	mutex_lock(&di->lock);
	ret = regmap_write(di->regmap, TCA9535_REGISTER_POLARITY_0, pol_0);
	if (ret) {
		dev_warn(di->dev, "Cannot write Configuration 0: %d\n", ret);
	}

	ret = regmap_write(di->regmap, TCA9535_REGISTER_POLARITY_1, pol_1);
	if (ret) {
		dev_warn(di->dev, "Cannot write Configuration 1: %d\n", ret);
	}

	mutex_unlock(&di->lock);
	return count;
}
static DEVICE_ATTR(polarity, 00660, polarity_show, polarity_store);


static struct attribute *tca9535_mux_attrs[] = {
	&dev_attr_address.attr,
	&dev_attr_input.attr,
	&dev_attr_out.attr,
	&dev_attr_config.attr,
	&dev_attr_polarity.attr,
	NULL,
};

static const struct attribute_group tca9535_mux_attr_group = {
	.attrs = tca9535_mux_attrs,
};


static int tca9535_mux_init(struct tca9535_device_info *di)
{
	return 0;
}

static int tca9535_mux_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct tca9535_device_info *di;
	int ret;
	char *name;
	int num;

	mutex_lock(&mux_mutex);
	num = idr_alloc(&mux_id, client, 0, 0, GFP_KERNEL);
	mutex_unlock(&mux_mutex);
	if (num < 0) {
		dev_err(&client->dev, "Cannot init TCA9535\n");
		return num;
	}

	name = devm_kasprintf(&client->dev, GFP_KERNEL, "%s-%d", id->name, num);
	if (!name)
		goto err_mem;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		goto err_mem;
	
	di->id = num;
	di->dev = &client->dev;
	di->name = name;
	di->iic_address = client->addr;
	mutex_init(&di->lock);

	i2c_set_clientdata(client, di);
	dev_info(&client->dev, "Init TCA9535: %s\n", di->name);

	di->pdev = platform_device_register_simple(MODULE_DEVICE_NAME, di->id, NULL, 0);
	if (IS_ERR(di->pdev)) {
		ret = PTR_ERR(di->pdev);
		goto err_failed;
	}

	di->regmap = devm_regmap_init_i2c(client, &tca9535_regmap_config);
	if (IS_ERR(di->regmap)) {
		dev_err(di->dev, "Cannot create regmap\n");
		ret = PTR_ERR(di->regmap);
		goto err_failed;
	}

	regcache_mark_dirty(di->regmap);

	platform_set_drvdata(di->pdev, di);

	ret = sysfs_create_group(&di->pdev->dev.kobj, &tca9535_mux_attr_group);
	if (ret) {
		dev_err(di->dev, "Cannot create sysfs\n");
		goto err_failed;
	}

	ret = tca9535_mux_init(di);
	if (ret) {
		dev_err(di->dev, "Cannot init TCA9535\n");
		goto err_failed;
	}

	return 0;

err_mem:
	ret = -ENOMEM;

err_failed:
	mutex_lock(&mux_mutex);
	idr_remove(&mux_id, num);
	mutex_unlock(&mux_mutex);

	return ret;
}

static int tca9535_mux_remove(struct i2c_client *client)
{
	struct tca9535_device_info *di = i2c_get_clientdata(client);
	dev_info(&client->dev, "Remove TCA9535: %s\n", di->name);
	sysfs_remove_group(&di->pdev->dev.kobj, &tca9535_mux_attr_group);
	
	platform_device_unregister(di->pdev);

	mutex_lock(&mux_mutex);
	idr_remove(&mux_id, di->id);
	mutex_unlock(&mux_mutex);
	return 0;
}

static const struct i2c_device_id tca9535_mux_id_table[] = {
	{ "tca9535-mux", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tca9535_mux_id_table);

static const struct of_device_id tca9535_mux_of_match_table[] = {
    { .compatible = "ti,tca9535-mux", },
    {}
};
MODULE_DEVICE_TABLE(of, tca9535_mux_of_match_table);

static struct i2c_driver tca9535_mux_driver = {
	.driver = {
        .name = MODULE_DEVICE_NAME,
        .of_match_table = tca9535_mux_of_match_table
    },
	.id_table = tca9535_mux_id_table,
    .probe = tca9535_mux_probe,
	.remove = tca9535_mux_remove,
};

module_i2c_driver(tca9535_mux_driver);


MODULE_LICENSE("GPL");       // License type
MODULE_AUTHOR("Vladimir Meshkov <glavmonter@gmail.com>");  // Author of the module
MODULE_DESCRIPTION("TCA9535 Multiplexer output only driver");
MODULE_VERSION("1.0");
