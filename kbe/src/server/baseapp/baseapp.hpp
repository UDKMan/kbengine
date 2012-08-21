/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2012 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef __BASEAPP_H__
#define __BASEAPP_H__
	
// common include	
#include "base.hpp"
#include "proxy.hpp"
#include "server/entity_app.hpp"
#include "server/pendingLoginmgr.hpp"
#include "server/forward_messagebuffer.hpp"
#include "network/endpoint.hpp"

//#define NDEBUG
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{

namespace Mercury{
	class Channel;
}

class Proxy;
class Baseapp :	public EntityApp<Base>, 
				public Singleton<Baseapp>
{
public:
	enum TimeOutType
	{
		TIMEOUT_CHECK_STATUS = TIMEOUT_ENTITYAPP_MAX + 1,
		TIMEOUT_MAX
	};
	
	Baseapp(Mercury::EventDispatcher& dispatcher, 
		Mercury::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~Baseapp();
	
	virtual bool installPyModules();
	virtual void onInstallPyModules();
	virtual bool uninstallPyModules();

	bool run();
	
	/** 
		相关处理接口 
	*/
	virtual void handleTimeout(TimerHandle handle, void * arg);
	virtual void handleGameTick();
	void handleCheckStatusTick();

	/* 初始化相关接口 */
	bool initializeBegin();
	bool initializeEnd();
	void finalise();
	
	virtual void onChannelDeregister(Mercury::Channel * pChannel);

	/** 网络接口
		dbmgr告知已经启动的其他baseapp或者cellapp的地址
		当前app需要主动的去与他们建立连接
	*/
	virtual void onGetEntityAppFromDbmgr(Mercury::Channel* pChannel, 
							int32 uid, 
							std::string& username, 
							int8 componentType, uint64 componentID, 
							uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport);
	
	/** 
		创建了一个entity回调
	*/
	virtual Base* onCreateEntityCommon(PyObject* pyEntity, ScriptModule* sm, ENTITY_ID eid);

	/** 
		创建一个entity 
	*/
	static PyObject* __py_createBase(PyObject* self, PyObject* args);
	static PyObject* __py_createBaseAnywhere(PyObject* self, PyObject* args);

	/**
		创建一个新的space 
	*/
	void createInNewSpace(Base* base, PyObject* cell);

	/** 
		在一个负载较低的baseapp上创建一个baseEntity 
	*/
	void createBaseAnywhere(const char* entityType, PyObject* params, PyObject* pyCallback);

	/** 收到baseappmgr决定将某个baseapp要求createBaseAnywhere的请求在本baseapp上执行 
		@param entityType	: entity的类别， entities.xml中的定义的。
		@param strInitData	: 这个entity被创建后应该给他初始化的一些数据， 需要使用pickle.loads解包.
		@param componentID	: 请求创建entity的baseapp的组件ID
	*/
	void onCreateBaseAnywhere(Mercury::Channel* pChannel, MemoryStream& s);

	/** 
		baseapp 的createBaseAnywhere的回调 
	*/
	void onCreateBaseAnywhereCallback(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);
	void _onCreateBaseAnywhereCallback(Mercury::Channel* pChannel, CALLBACK_ID callbackID, 
		std::string& entityType, ENTITY_ID eid, COMPONENT_ID componentID);

	/** 
		为一个baseEntity在制定的cell上创建一个cellEntity 
	*/
	void createCellEntity(EntityMailboxAbstract* createToCellMailbox, Base* base);
	
	/** 网络接口
		createCellEntity的cell实体创建成功回调。
	*/
	void onEntityGetCell(Mercury::Channel* pChannel, ENTITY_ID id, COMPONENT_ID componentID);

	/** 
		通知客户端创建一个proxy对应的实体 
	*/
	bool createClientProxies(Proxy* base, bool reload = false);

	/** 
		想dbmgr请求执行一个数据库命令
	*/
	static PyObject* __py_executeRawDatabaseCommand(PyObject* self, PyObject* args);
	void executeRawDatabaseCommand(const char* datas, uint32 size, PyObject* pycallback);
	void onExecuteRawDatabaseCommandCB(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		dbmgr发送初始信息
		startID: 初始分配ENTITY_ID 段起始位置
		endID: 初始分配ENTITY_ID 段结束位置
		startGlobalOrder: 全局启动顺序 包括各种不同组件
		startGroupOrder: 组内启动顺序， 比如在所有baseapp中第几个启动。
	*/
	void onDbmgrInitCompleted(Mercury::Channel* pChannel, 
		ENTITY_ID startID, ENTITY_ID endID, int32 startGlobalOrder, int32 startGroupOrder);

	/** 网络接口
		dbmgr广播global数据的改变
	*/
	void onBroadcastGlobalBasesChange(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		注册将要登录的账号, 注册后则允许登录到此网关
	*/
	void registerPendingLogin(Mercury::Channel* pChannel, std::string& accountName, 
		std::string& password, ENTITY_ID entityID);

	/** 网络接口
		新用户请求登录到网关上
	*/
	void loginGateway(Mercury::Channel* pChannel, std::string& accountName, std::string& password);

	/** 网络接口
		重新登录 快速与网关建立交互关系(前提是之前已经登录了， 
		之后断开在服务器判定该前端的Entity未超时销毁的前提下可以快速与服务器建立连接并达到操控该entity的目的)
	*/
	void reLoginGateway(Mercury::Channel* pChannel, std::string& accountName, 
		std::string& password, uint64 key, ENTITY_ID entityID);

	/**
	   登录失败
	   @failedcode: 失败返回码 MERCURY_ERR_SRV_NO_READY:服务器没有准备好, 
									MERCURY_ERR_ILLEGAL_LOGIN:非法登录, 
									MERCURY_ERR_NAME_PASSWORD:用户名或者密码不正确
	*/
	void loginGatewayFailed(Mercury::Channel* pChannel, std::string& accountName, MERCURY_ERROR_CODE failedcode);

	/** 网络接口
		从dbmgr获取到账号Entity信息
	*/
	void onQueryAccountCBFromDbmgr(Mercury::Channel* pChannel, std::string& accountName, std::string& password, std::string& datas);

	/** 网络接口
		通知客户端进入了cell（世界或者AOI)
	*/
	void onEntityEnterWorldFromCellapp(Mercury::Channel* pChannel, ENTITY_ID entityID);
	
	/**
		客户端自身进入世界了
	*/
	void onClientEntityEnterWorld(Proxy* base);

	/** 网络接口
		通知客户端离开了cell（世界或者AOI)
	*/
	void onEntityLeaveWorldFromCellapp(Mercury::Channel* pChannel, ENTITY_ID entityID);

	/** 网络接口
		通知客户端进入了某个space
	*/
	void onEntityEnterSpaceFromCellapp(Mercury::Channel* pChannel, ENTITY_ID entityID, SPACE_ID spaceID);

	/** 网络接口
		通知客户端离开了某个space
	*/
	void onEntityLeaveSpaceFromCellapp(Mercury::Channel* pChannel, ENTITY_ID entityID, SPACE_ID spaceID);

	/** 网络接口
		entity收到一封mail, 由某个app上的mailbox发起(只限与服务器内部使用， 客户端的mailbox调用方法走
		onRemoteCellMethodCallFromClient)
	*/
	void onEntityMail(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);
	
	/** 网络接口
		client访问entity的cell方法
	*/
	void onRemoteCallCellMethodFromClient(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);
protected:
	TimerHandle							loopCheckTimerHandle_;
	GlobalDataClient*					pGlobalBases_;								// globalBases

	// 记录登录到服务器但还未处理完毕的账号
	PendingLoginMgr						pendingLoginMgr_;

	ForwardComponent_MessageBuffer		forward_messagebuffer_;
};

}
#endif
