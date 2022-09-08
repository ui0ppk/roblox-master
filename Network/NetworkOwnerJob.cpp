
#include "NetworkOwnerJob.h"

#include "NetworkSettings.h"
#include "Players.h"
#include "Server.h"
#include "ServerReplicator.h"
#include "Util.h"

#include "V8DataModel/DataModel.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8World/Assembly.h"
#include "V8World/Mechanism.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"
#include "V8World/SpatialFilter.h"
#include "V8World/DistributedPhysics.h"
#include "Network/NetworkOwner.h"

using namespace RBX;
using namespace RBX::Network;

DYNAMIC_FASTFLAG(CyclicExecutiveForServerTweaks)


#define DEC_SIM_RADIUS_FPS 15.0f
#define INC_SIM_RADIUS_FPS 17.5f

NetworkOwnerJob::NetworkOwnerJob(shared_ptr<DataModel> dataModel)
	: DataModelJob("Distributed Physics Ownership", DataModelJob::Write, false, dataModel, Time::Interval(0))
	, dataModel(dataModel)
	, networkOwnerRate(NetworkSettings::singleton().networkOwnerRate)
{
	cyclicExecutive = true;
}

Time::Interval NetworkOwnerJob::sleepTime(const Stats& stats)
{
	return computeStandardSleepTime(stats, networkOwnerRate);
}

TaskScheduler::Job::Error NetworkOwnerJob::error(const Stats& stats)
{
	if (DFFlag::CyclicExecutiveForServerTweaks)
	{
		return TaskScheduler::Job::computeStandardErrorCyclicExecutiveSleeping(stats, networkOwnerRate);
	}
	return TaskScheduler::Job::computeStandardError(stats, networkOwnerRate);
}


TaskScheduler::StepResult NetworkOwnerJob::stepDataModelJob(const Stats& stats) 
{
	if(shared_ptr<DataModel> safeDataModel = dataModel.lock()){
		DataModel::scoped_write_request request(safeDataModel.get());
		RBXASSERT(Server::serverIsPresent(safeDataModel.get()));

		if (Server* server = ServiceProvider::find<Server>(safeDataModel.get()))
		{
			updatePlayerLocations(server);

			SpatialFilter* spatialFilter = safeDataModel->getWorkspace()->getWorld()->getSpatialFilter();	// go through all parts/primitives in the spatial filter server side
			for (size_t phase = 0; phase < Assembly::NUM_PHASES; phase++)
			{
				const SpatialFilter::AssemblySet& assemblies = spatialFilter->getAssemblies(static_cast<Assembly::FilterPhase>(phase));
				SpatialFilter::AssemblySet::const_iterator it;
				for (it = assemblies.begin(); it != assemblies.end(); ++it)
				{
					Assembly* assembly = *it;
					RBXASSERT(Mechanism::isMovingAssemblyRoot(assembly));
					PartInstance* part = PartInstance::fromPrimitive(assembly->getAssemblyPrimitive());
					updateNetworkOwner(part);
				}
			}
		}
		return TaskScheduler::Stepped;
	}
	return TaskScheduler::Done;
}

////////////////////////////////////////////////////////////////////////

void NetworkOwnerJob::updatePlayerLocations(Server* server)
{
	clientMap.clear();

	for (size_t i = 0; i < server->numChildren(); ++i)
	{
		if(ServerReplicator* proxy = Instance::fastDynamicCast<ServerReplicator>(server->getChild(i))){
			Region2::WeightedPoint weightedPoint;
			if (const PartInstance* head = proxy->readPlayerSimulationRegion(weightedPoint))
			{
				const Primitive* headPrim = head->getConstPartPrimitive();
				const Mechanism* headMechanism = headPrim->getConstMechanism();
				LEGACY_ASSERT(Mechanism::isMovingAssemblyRoot(headPrim->getConstAssembly()));
		
				RBX::SystemAddress rbxAddress = RakNetToRbxAddress(proxy->remotePlayerId);
				ClientMapPair pair(rbxAddress, ClientLocation(weightedPoint, proxy, headMechanism));
				clientMap.insert(pair);

				// reset part count
				proxy->numPartsOwned = 0;
			}
		}
	}
}

// TODO - untangle, make more efficient

void NetworkOwnerJob::updateNetworkOwner(PartInstance* part)
{
	RBXASSERT(part->getPartPrimitive());
	RBXASSERT(Assembly::isAssemblyRootPrimitive(part->getPartPrimitive()));
	RBXASSERT(part->getPartPrimitive()->getAssembly()->getAssemblyPrimitive() == part->getPartPrimitive());

    RBX::SystemAddress owner = part->getNetworkOwner();  
    
    if (part->getNetworkOwnershipRule() == NetworkOwnership_Manual)
	{
		ClientMapIt clientLoc = clientMap.find(owner);
		if ((clientLoc == clientMap.end() && !Network::Players::findPlayerWithAddress(owner, part)) && owner != NetworkOwner::Server())
		{
			resetNetworkOwner(part, NetworkOwner::Server());
			part->setNetworkOwnershipAuto();
		}
		else
		{
			return;
		}
	}
    
    
	if ((owner == NetworkOwner::Unassigned()) || (owner == NetworkOwner::ServerUnassigned())) {

		RBX::SystemAddress newOwner = NetworkOwner::Server();

		// check to see if this part belong to a player character
		if(ModelInstance* m = Instance::fastDynamicCast<ModelInstance>(part->getParent()))
		{
			if (Players::getPlayerFromCharacter(m))
			{
				ClientMapConstIt closestClient = findClosestClientToPart(part);

				if (closestClient != clientMap.end())
				{
					newOwner = closestClient->first;

					closestClient->second.clientProxy->numPartsOwned++;
				}
			}
		}

		resetNetworkOwner(part, newOwner);
		return;
	}

	ClientMapConstIt currentClient = clientMap.find(owner);		// will be clientMap.end() for server
    ClientMapConstIt closestClient;

    bool currentOk = false;
    bool closestOk = false;

    // projectile special handling to prevent the ownership change from one client to another
    if (!currentClient->second.overloaded)
    {
    	if (part->isProjectile() && currentClient != clientMap.end()) {
		    currentClient->second.clientProxy->numPartsOwned++;
		    return;
        }
	}

    closestClient = findClosestClientToPart(part);
    currentOk = ((currentClient != clientMap.end()) && clientCanSimulate(part, currentClient));
    closestOk = ((closestClient != currentClient) && (closestClient != clientMap.end()) && part->networkOwnerTimeUp() && clientCanSimulate(part, closestClient));

	if (currentOk) {
		if (closestOk && switchOwners(part, currentClient, closestClient)) 
		{
			resetNetworkOwner(part, closestClient->first);
			closestClient->second.clientProxy->numPartsOwned++;
		}
		else
			currentClient->second.clientProxy->numPartsOwned++;
		return;
	}
	// !currentOk
	if (closestOk) {	// no timer needed if current is not ok
		resetNetworkOwner(part, closestClient->first);
		closestClient->second.clientProxy->numPartsOwned++;
		return;
	}

	resetNetworkOwner(part, NetworkOwner::Server());
}

void NetworkOwnerJob::resetNetworkOwner(PartInstance* part, const RBX::SystemAddress value)
{
	if (part->getNetworkOwner() != value) 
	{
		part->setNetworkOwnerAndNotify(value);

		part->raknetTime = 0;   // clear the last update timestamp
		// 3 seconds is too slow for iPad clients to push parts to server when stressed
		//float delayTime = (value == NetworkOwner::Server()) ? 0.1f : 3.0f;		// i.e. - a server part only waits 0.1 second before it can switch.  A client part waits 3 seconds.
		float delayTime = 0.1f;
		part->resetNetworkOwnerTime(delayTime);
	}
}

NetworkOwnerJob::ClientMapConstIt NetworkOwnerJob::findClosestClientToPart(PartInstance* part) {
	return findClosestClient(part->getCoordinateFrame().translation.xz());
}

NetworkOwnerJob::ClientMapConstIt NetworkOwnerJob::findClosestClient(const Vector2& testLocation)
{
	ClientMapConstIt answer = clientMap.end();
	float bestError = 1e20f;

	for (ClientMapConstIt it = clientMap.begin(); it != clientMap.end(); ++it)
	{
		float temp = Region2::getRelativeError(testLocation, it->second.clientPoint);
		if (temp < bestError) {
			bestError = temp;
			answer = it;

			// can't get closer then 0
			if (temp == 0.0f)
				return answer;
		}
	}

	return answer;
}

/*
	For a client to be able to simulate a part/assembly, it must be

	Mechanisms that DO NOT include the client's character:		Within the client's simulation region

	Mechanisms that DO include the client's character:			
							Standing on top of something simulated by someone else?		... never simulate
							NOT standing on something simulated by others:				Always simulate
*/


bool NetworkOwnerJob::clientCanSimulate(PartInstance* part, ClientMapConstIt testLocation)
{
	RBXASSERT(testLocation != clientMap.end());

//	return false;

	if (isClientCharacterMechanism(part, testLocation)) {
		return true;
	}
	else {
		Vector2 partLoc = part->getCoordinateFrame().translation.xz();
		return Region2::pointInRange(partLoc, testLocation->second.clientPoint, DistributedPhysics::SERVER_SLOP());
	}
}

bool NetworkOwnerJob::isClientCharacterMechanism(PartInstance* part, ClientMapConstIt testLocation)
{
	const Mechanism* characterMechanism = testLocation->second.characterMechanism;

	const Primitive* dPhysPrim = part->getConstPartPrimitive();
	const Mechanism* dPhysMechanism = dPhysPrim->getConstMechanism();
		
	RBXASSERT(Mechanism::isMovingAssemblyRoot(dPhysPrim->getConstAssembly()));
	
	if (characterMechanism == dPhysMechanism) {
		return true;
	}
	else {
		return false;
	}
}

bool NetworkOwnerJob::switchOwners(PartInstance* part, ClientMapConstIt currentOwner, ClientMapConstIt possibleNewOwner)
{
	RBXASSERT(currentOwner != clientMap.end());
	RBXASSERT(possibleNewOwner != clientMap.end());
	RBXASSERT(part->getNetworkOwner() == currentOwner->first);
	RBXASSERT(part->getNetworkOwner() != possibleNewOwner->first);

	if (isClientCharacterMechanism(part, currentOwner)) {			// no switch allowed if current mechanism is current owner mechanism
		return false;
	}
	else {
		return Region2::closerToOtherPoint(	part->getCoordinateFrame().translation.xz(),
											currentOwner->second.clientPoint,
											possibleNewOwner->second.clientPoint,
											DistributedPhysics::SERVER_SLOP()	);
	}
}

void NetworkOwnerJob::invalidateProjectileOwnership(RBX::SystemAddress addr)
{
    ClientMapIt clientLoc = clientMap.find(addr);
    if (clientLoc != clientMap.end())
    {
        clientLoc->second.overloaded = true;
    }
}
