#pragma once

#include "v8datamodel/contentprovider.h"

namespace RBX {

		class AsyncResult
		{
		public:
			AsyncResult()
				: reqResult(RBX::AsyncHttpQueue::Succeeded)
			{
			};
			
			// make result always more restrictive only.
			// Succeeded < Waiting < Failed.
			void returnResult(RBX::AsyncHttpQueue::RequestResult reqResult)
			{
				switch(reqResult)
				{
				case RBX::AsyncHttpQueue::Succeeded: 
					break;
				case RBX::AsyncHttpQueue::Waiting: 
					if(this->reqResult == RBX::AsyncHttpQueue::Succeeded)
					{
						this->reqResult = reqResult;
					}
					break;
				case RBX::AsyncHttpQueue::Failed:
					this->reqResult = reqResult;
					break;
				}
			}

			void returnWaitingFor(const RBX::ContentId& id)
			{
				returnResult(RBX::AsyncHttpQueue::Waiting);
				waitingFor.push_back(id);
			}

			RBX::AsyncHttpQueue::RequestResult reqResult;
			std::vector<RBX::ContentId> waitingFor;
		};


}