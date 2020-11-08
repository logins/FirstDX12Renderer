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

// Gets and eventually reates a device from the main adapter found.
// We are assuming to have only one device in our code.
Device& GetDevice();

} }
#endif // Device_h__
