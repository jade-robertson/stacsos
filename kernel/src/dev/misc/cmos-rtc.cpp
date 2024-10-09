#include <stacsos/kernel/dev/misc/cmos-rtc.h>

using namespace stacsos::kernel::dev;
using namespace stacsos::kernel::dev::misc;
using namespace stacsos::kernel::arch::x86;

device_class cmos_rtc::cmos_rtc_device_class(rtc::rtc_device_class, "cmos-rtc");


unsigned char get_RTC_register(int reg) {
    ioports::cmos_select::write8(reg);
    return ioports::cmos_data::read8();
}

rtc_timepoint cmos_rtc::read_timepoint() { 
    rtc_timepoint t;

    //Incomplete implementaion
    t.seconds = get_RTC_register(0x00);
    t.minutes = get_RTC_register(0x02);
    t.hours = get_RTC_register(0x04);
    t.day_of_month = get_RTC_register(0x07);
    t.month = get_RTC_register(0x08);
    t.year = get_RTC_register(0x09);
    return t;
 };
