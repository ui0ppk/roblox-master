//
// XboxLive stuff
//
// Very toxic, do not include in headers!
//

#include <collection.h>
#include <string>
#include <vector>
#include "v8datamodel/PlatformService.h"

class UserIDTranslator;
class XboxPlatform;

using namespace Microsoft::Xbox::Services;
using namespace Microsoft::Xbox::Services::Multiplayer;

int getFriends(std::string* ret, UserIDTranslator* translator, Microsoft::Xbox::Services::XboxLiveContext^ xboxLiveContext );
int getParty(std::vector<std::string>* result, bool filter, UserIDTranslator* translator, Microsoft::Xbox::Services::XboxLiveContext^ xboxLiveContext);

void xboxEvents_init();
void xboxEvents_shutdown();
RBX::AwardResult xboxEvents_send( Microsoft::Xbox::Services::XboxLiveContext^ xboxLiveContext, const char* evtName, double* value = NULL ); // reentrant
