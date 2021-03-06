#include "osc/commands/Quit.hpp"

#include "osc/Address.hpp"
#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

Quit::Quit(osc::Dispatcher* dispatcher): Command(dispatcher) {}

Quit::~Quit() {}

void Quit::processMessage(int /* argc */, lo_arg** /* argv */, const char* /* types */, lo_address address) {
    std::shared_ptr<Address> origin(new Address(address));
    m_dispatcher->callQuitHandler(origin);
}

} // namespace commands
} // namespace osc
} // namespace scin
