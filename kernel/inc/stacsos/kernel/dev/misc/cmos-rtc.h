#pragma once

#include <stacsos/kernel/dev/misc/rtc.h>
#include <stacsos/kernel/arch/x86/pio.h>
namespace stacsos::kernel::dev::misc {
class cmos_rtc : public rtc {
public:
	static device_class cmos_rtc_device_class;

	cmos_rtc(bus &owner)
		: rtc(cmos_rtc_device_class, owner)
	{
	}

	virtual void configure() override { }

	virtual rtc_timepoint read_timepoint() override;
};
} // namespace stacsos::kernel::dev::misc
