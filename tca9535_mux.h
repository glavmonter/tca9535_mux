#ifndef __LINUX_TCA9535_MUX_H__
#define __LINUX_TCA9535_MUX_H__

struct tca9535_device_info {
	struct device *dev;
	struct platform_device *pdev;
	struct mutex lock;
	struct regmap *regmap;
	int id;
	int iic_address;
	const char *name;

};

#endif
