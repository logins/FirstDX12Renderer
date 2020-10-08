#ifndef Device_h__
#define Device_h__

#include <memory>

namespace GEPUtils { namespace Graphics {

/*!
 * \class Device
 *
 * \brief Platform-agnostic representation of a graphics device.
 *
 * \author Riccardo Loggini
 * \date July 2020
 */
class Device
{
public:
	Device();
private:
	bool m_IsMainDevice=false;
};

// Creates a device from the main adapter found
std::unique_ptr<Device> CreateDevice();

} }
#endif // Device_h__
