/*
 * CMOS Real-time Clock
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (3)
 */

/*
 * STUDENT NUMBER: s1621503
 */
#include <infos/drivers/timer/rtc.h>

using namespace infos::drivers;
using namespace infos::drivers::timer;

// necessary imports
#include <infos/util/lock.h>
#include <infos/util/printf.h>
#include <arch/x86/pio.h>
#include <infos/kernel/log.h>

using namespace infos::util;
using namespace infos::arch::x86;
using namespace infos::kernel;

class CMOSRTC : public RTC
{
  public:
	static const DeviceClass CMOSRTCDeviceClass;

	const DeviceClass &device_class() const override
	{
		return CMOSRTCDeviceClass;
	}

	enum
	{
		cmos_address = 0x70,
		cmos_data = 0x71
	};

	/**
	 * Obtains the value from the requested RTC register.
	 * @param reg The register to request.
	 * @return Returns the value in the register.
	 */
	uint8_t get_rtc_register(uint8_t reg)
	{
		__outb(cmos_address, reg);
		return __inb(cmos_data);
	}

	/**
	 * Converts from binary coded decimal to decimal.
	 * @param bcd The value to be converted
	 * @return Returns the converted value.
	 */
	uint8_t bcd2dec(uint8_t bcd) {
		uint8_t hi = ((bcd & 0b11110000) >> 4) * 10;
		uint8_t lo =   bcd & 0b00001111;
		return hi + lo;
	}

	/**
	 * Interrogates the RTC to read the current date & time.
	 * @param tp Populates the tp structure with the current data & time, as
	 * given by the CMOS RTC device.
	 */
	void read_timepoint(RTCTimePoint &tp) override
	{
		syslog.messagef(LogLevel::DEBUG, "reading rtc timepoint");

		// request interrupt lock
		UniqueIRQLock l;

		// set update bit mask
		uint8_t update_mask = 1 << 7;
		// set size of cmos array (up to register B + 1)
		uint8_t rtc_size = 0xb + 1;
		uint8_t cmos_mem[rtc_size];

		// wait for update start
		while (!(get_rtc_register(0xa) & update_mask));
		// update has begun, wait until finished
		while (get_rtc_register(0xa) & update_mask);

		syslog.messagef(LogLevel::DEBUG, "rtc update complete!");

		for (uint8_t i = 0; i < rtc_size; i++) {
			cmos_mem[i] = get_rtc_register(i);
		}
		
		// mask and isolate bit 2
		uint8_t encoding = get_rtc_register(0xb) & 1 << 2;

		uint8_t register_offsets[] = {0x0, 0x2, 0x4, 0x7, 0x8, 0x9};
		if (!encoding) {
			// binary coded decimal
			syslog.messagef(LogLevel::DEBUG, "binary coded decimal detected - converting...");
			
			for (uint8_t r : register_offsets) {
				uint8_t c = bcd2dec(cmos_mem[r]);
				// syslog.messagef(LogLevel::DEBUG, "converted #%d: %8b --> %8b", r, cmos_mem[r], c);
				cmos_mem[r] = c;
			}
		}
		
		tp.seconds = cmos_mem[0x0];
		tp.minutes = cmos_mem[0x2];
		tp.hours = cmos_mem[0x4];

		tp.day_of_month = cmos_mem[0x7];
		tp.month = cmos_mem[0x8];
		tp.year = cmos_mem[0x9];

		syslog.messagef(LogLevel::DEBUG, "done reading rtc timepoint!");
	}
};

const DeviceClass CMOSRTC::CMOSRTCDeviceClass(RTC::RTCDeviceClass, "cmos-rtc");

RegisterDevice(CMOSRTC);
