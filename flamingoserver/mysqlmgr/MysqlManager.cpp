#include "MysqlManager.h"
#include <sstream>
#include "../base/Singleton.h"
#include "MysqlThrdMgr.h"
#include <iostream>

#include "../deng.h"

CMysqlManager::CMysqlManager(void)
{
	// TODO: m_strCharactSet可以放在初始化列表中初始化
	m_strCharactSet = "utf8";

	// 初始化表
	// 1. t_user
	{
		STableInfo info;
		// name type desc?
		info.m_strName = "t_user";
		info.m_mapField["f_id"] = {"f_id", "bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID'", "bigint(20)"};
		info.m_mapField["f_user_id"] = {"f_user_id", "bigint(20) NOT NULL COMMENT '用户ID'", "bigint(20)"};
		info.m_mapField["f_username"] = {"f_username", "varchar(64) NOT NULL COMMENT '用户名'", "varchar(64)"};
		info.m_mapField["f_nickname"] = {"f_nickname", "varchar(64) NOT NULL COMMENT '用户昵称'", "varchar(64)"};
		info.m_mapField["f_password"] = {"f_password", "varchar(64) NOT NULL COMMENT '用户密码'", "varchar(64)"};
		info.m_mapField["f_facetype"] = {"f_facetype", "int(10) DEFAULT 0 COMMENT '用户头像类型'", "int(10)"};
		info.m_mapField["f_customface"] = {"f_customface", "varchar(32) DEFAULT NULL COMMENT '自定义头像名'", "varchar(32)"};
		info.m_mapField["f_customfacefmt"] = {"f_customfacefmt", "varchar(6) DEFAULT NULL COMMENT '自定义头像格式'", "varchar(6)"};
		info.m_mapField["f_gender"] = {"f_gender", "int(2)  DEFAULT 0 COMMENT '性别'", "int(2)"};
		info.m_mapField["f_birthday"] = {"f_birthday", "bigint(20)  DEFAULT 19900101 COMMENT '生日'", "bigint(20)"};
		info.m_mapField["f_signature"] = {"f_signature", "varchar(256) DEFAULT NULL COMMENT '地址'", "varchar(256)"};
		info.m_mapField["f_address"] = {"f_address", "varchar(256) DEFAULT NULL COMMENT '地址'", "varchar(256)"};
		info.m_mapField["f_phonenumber"] = {"f_phonenumber", "varchar(64) DEFAULT NULL COMMENT '电话'", "varchar(64)"};
		info.m_mapField["f_mail"] = {"f_mail", "varchar(256) DEFAULT NULL COMMENT '邮箱'", "varchar(256)"};
		info.m_mapField["f_owner_id"] = {"f_owner_id", "bigint(20) DEFAULT 0 COMMENT '群账号群主userid'", "bigint(20)"};
		info.m_mapField["f_teaminfo"] = {"f_teaminfo", "blob default null comment '好友分组信息'", "blob"};
		info.m_mapField["f_register_time"] = {"f_register_time", "datetime NOT NULL COMMENT '注册时间'", "datetime"};
		info.m_mapField["f_remark"] = {"f_remark", "varchar(64) NULL COMMENT '备注'", "varchar(64)"};
		info.m_mapField["f_update_time"] = {"f_update_time", "timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间'", "timestamp"};

		info.m_strKeyString = "PRIMARY KEY (f_user_id), INDEX f_user_id (f_user_id), KEY  f_id  ( f_id )";
		m_vecTableInfo.push_back(info);

		// info.m_mapField["AccountID"] = { "AccountID", "integer NOT NULL AUTO_INCREMENT PRIMARY KEY", "int(11)" };
		// info.m_mapField["MobilePhone"] = { "MobilePhone", "bigint NOT NULL", "bigint(20)" };
		// info.m_mapField["NickName"] = { "NickName", "char(65) not null", "char(65)" };
		// info.m_mapField["PassWord"] = { "PassWord", "char(65) not null", "char(65)" };
		////info.m_mapField["Friend"] = { "Friend", "blob not null", "blob" };
		//      info.m_mapField["Friend"] = { "Friend", "blob default null", "blob" };
		// info.m_mapField["PersonInfo"] = { "PersonInfo", "blob default null", "blob" };
		//
		// info.m_strKeyString = "UNIQUE KEY(MobilePhone)";
		// m_vecTableInfo.push_back(info);
	}

	// 2. t_user_relationship
	{
		STableInfo info;
		// name type desc?
		info.m_strName = "t_user_relationship";
		info.m_mapField["f_id"] = {"f_id", "bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID'", "bigint(20)"};
		info.m_mapField["f_user_id1"] = {"f_user_id1", "bigint(20) NOT NULL COMMENT '用户ID'", "bigint(20)"};
		info.m_mapField["f_user_id2"] = {"f_user_id2", "bigint(20) NOT NULL COMMENT '用户ID'", "bigint(20)"};
		info.m_mapField["f_user1_teamname"] = {"f_user1_teamname", "VARCHAR(32) NOT NULL DEFAULT 'My Friends' COMMENT '用户2在用户1的好友分组名称'", "VARCHAR(32)"};
		info.m_mapField["f_user2_teamname"] = {"f_user2_teamname", "VARCHAR(32) NOT NULL DEFAULT 'My Friends' COMMENT '用户1在用户2的好友分组名称'", "VARCHAR(32)"};
		info.m_mapField["f_user1_markname"] = {"f_user1_markname", "VARCHAR(32) COMMENT '用户2在用户1的备注名称'", "VARCHAR(32)"},
		info.m_mapField["f_user2_markname"] = {"f_user2_markname", "VARCHAR(32) COMMENT '用户1在用户2的备注名称'", "VARCHAR(32)"},
		info.m_mapField["f_remark"] = {"f_remark", "varchar(64) NULL COMMENT '备注'", "varchar(64)"};
		info.m_mapField["f_update_time"] = {"f_update_time", "timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间'", "timestamp"};

		info.m_strKeyString = "PRIMARY KEY (f_id), INDEX f_id (f_id)";
		m_vecTableInfo.push_back(info);

		// info.m_mapField["AccountID"] = { "AccountID", "integer NOT NULL AUTO_INCREMENT PRIMARY KEY", "int(11)" };
		// info.m_mapField["MobilePhone"] = { "MobilePhone", "bigint NOT NULL", "bigint(20)" };
		// info.m_mapField["NickName"] = { "NickName", "char(65) not null", "char(65)" };
		// info.m_mapField["PassWord"] = { "PassWord", "char(65) not null", "char(65)" };
		////info.m_mapField["Friend"] = { "Friend", "blob not null", "blob" };
		//      info.m_mapField["Friend"] = { "Friend", "blob default null", "blob" };
		// info.m_mapField["PersonInfo"] = { "PersonInfo", "blob default null", "blob" };
		//
		// info.m_strKeyString = "UNIQUE KEY(MobilePhone)";
		// m_vecTableInfo.push_back(info);
	}

	// 3. t_chatmsg //deng:一条消息需要知道：f_senderid， f_targetid， f_msgcontent， f_create_time（系统会自动添加）；
	{
		STableInfo chat;
		chat.m_strName = "t_chatmsg";
		chat.m_mapField["f_id"] = {"f_id", "bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID'", "bigint(20)"};
		chat.m_mapField["f_senderid"] = {"f_senderid", "bigint(20) NOT NULL COMMENT '发送者id'", "bigint(20)"};
		chat.m_mapField["f_targetid"] = {"f_targetid", "bigint(20) NOT NULL COMMENT '接收者id'", "bigint(20)"};
		chat.m_mapField["f_msgcontent"] = {"f_msgcontent", "BLOB NOT NULL COMMENT '聊天内容'", "BLOB"};
		chat.m_mapField["f_create_time"] = {"f_create_time", "timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间'", "timestamp"}; // deng: 写入时系统自动把当前时间写进去
		chat.m_mapField["f_remark"] = {"f_remark", "varchar(64) NULL COMMENT '备注'", "varchar(64)"};																	  // deng: 暂时没用

		chat.m_strKeyString = "PRIMARY KEY (f_id), INDEX f_id (f_id)";
		m_vecTableInfo.push_back(chat);
	}
}

CMysqlManager::~CMysqlManager(void)
{
}

// deng: 查看有没有需要的数据库和表， 没有则创建数据库和表。
bool CMysqlManager::init(const char *host, const char *user, const char *pwd, const char *dbname)
{
	m_strHost = host;
	m_strUser = user;
	//数据库密码可能为空
	if (pwd != NULL)
		m_strPassword = pwd;
	m_strDataBase = dbname;

	//注意：检查数据库是否存在时，需要将数据库名称设置为空
	m_poConn.reset(new CDatabaseMysql());

	// deng: 先得到 mysql 连接。
	if (!m_poConn->initialize(m_strHost, m_strUser, m_strPassword, ""))
	{
		LOG_ERROR("CMysqlManager::init : m_poConn->initialize ERROR");
		// std::cout<< "第一次初始化"<<std::endl;
		// LOG_FATAL << "CMysqlManager::Init failed, please check params(" << m_strHost << ", " << m_strUser << ", " << m_strPassword << ")";
		return false;
	}

	////////////////////// 1. 检查库是否存在 /////////////////////////
	if (!isDBExist())
	{
		LOG_INFO("数据库系统：数据库不存在，创建数据库");
		if (!createDB())
		{
			LOG_INFO("数据库系统：数据库创建 失败");
			// std::cout<< "createDB 失败"<<std::endl;
			return false;
		}
		LOG_INFO("数据库系统：数据库创建 成功");
	}

	//再次确定是否可以连接上数据库
	m_poConn.reset(new CDatabaseMysql());
	if (!m_poConn->initialize(m_strHost, m_strUser, m_strPassword, m_strDataBase))
	{
		LOG_ERROR("CMysqlManager::init : m_poConn->initialize ERROR");
		// LOG_FATAL << "CMysqlManager::Init failed, please check params(" << m_strHost << ", " << m_strUser
		//	<< ", " << m_strPassword << ", " << m_strDataBase << ")";
		return false;
	}

	////////////////////// 2. 检查库中表是否正确 /////////////////////////
	for (size_t i = 0; i < m_vecTableInfo.size(); i++)
	{
		STableInfo table = m_vecTableInfo[i];
		if (!checkTable(table))
		{
			LOG_ERROR("CMysqlManager::init : checkTable ERROR");
			// LOG_FATAL << "CMysqlManager::Init, table check failed : " << table.m_strName;
			return false;
		}
	}
	////////////////////// 2. 检查库中表是否正确 /////////////////////////

	m_poConn.reset();
	return true;
}

bool CMysqlManager::isDBExist()
{
	if (NULL == m_poConn)
	{
		return false;
	}

	// deng: m_poConn->query 里调用了一次 QueryResult:: nextRow
	QueryResult *pResult = m_poConn->query("show databases");
	if (NULL == pResult)
	{
		// LOGI << "CMysqlManager::_IsDBExist, no database(" << m_strDataBase << ")";
		return false;
	}

	// deng: 得到当前行
	Field *pRow = pResult->fetch();
	while (pRow != NULL)
	{

		// std::cout << "前 pRow = " << pRow  << std::endl;

		// deng: 因为数据库只有一个字段
		std::string name = pRow[0].getString();
		// std::cout << "pRow[0].getString() = " << pRow[0].getString()  << std::endl;

		if (name == m_strDataBase)
		{
			// LOGI << "CMysqlManager::_IsDBExist, find database(" << m_strDataBase << ")";
			pResult->endQuery();
			return true;
		}

		if (pResult->nextRow() == false)
		{
			break;
		}

		pRow = pResult->fetch();
		// std::cout << "后 pRow = " << pRow  << std::endl;
	}

	// LOGI << "CMysqlManager::_IsDBExist, no database(" << m_strDataBase << ")";
	pResult->endQuery();

	delete pResult;
	return false;
}

bool CMysqlManager::createDB()
{
	if (NULL == m_poConn)
	{
		return false;
	}

	uint32_t uAffectedCount = 0;
	int nErrno = 0;

	std::stringstream ss;
	ss << "create database " << m_strDataBase;

	// deng: m_poConn->execute 返回执行这条语句影响到的 行数
	if (m_poConn->execute(ss.str().c_str(), uAffectedCount, nErrno))
	{

		if (uAffectedCount == 1)
		{
			// LOGI << "CMysqlManager::_CreateDB, create database " <<
			//	m_strDataBase << " success";
			return true;
		}
	}
	else
	{
		// LOGE << "CMysqlManager::_CreateDB, create database " << m_strDataBase << " failed("
		//	<< nErrno << ")";
		return false;
	}
	return false;
}

bool CMysqlManager::checkTable(const STableInfo &table)
{
	if (NULL == m_poConn)
	{
		return false;
	}

	// degn: "\t\r\n " 里任何一个字符开头都不行
	if (table.m_strName.find_first_not_of("\t\r\n ") == std::string::npos)
	{
		// LOGW << "CMysqlManager::_CheckTable, tale info not valid";
		return true;
	}

	std::stringstream ss;
	ss << "desc " << table.m_strName;
	QueryResult *pResult = m_poConn->query(ss.str());
	if (NULL == pResult)
	{
		// LOGI << "CMysqlManager::_CheckTable, no table" << table.m_strName << ", begin create.....";
		LOG_INFO("创建表开始 table = %s", table.m_strName);
		if (createTable(table))
		{
			LOG_INFO("创建表成功");
			// LOGI << "CMysqlManager::_CheckTable, " << table.m_strName << ", end create.....";
			return true;
		}
		LOG_INFO("创建表失败");
		return false;
	}
	else // 检查字段是否匹配， 暂时只检查是否存在， 还需进一步看类型是否需要修改
	{
		std::map<std::string, std::string> mapOldTable;
		Field *pRow = pResult->fetch();
		while (pRow != NULL)
		{
			std::string name = pRow[0].getString();
			std::string type = pRow[1].getString();
			mapOldTable[name] = type;

			if (pResult->nextRow() == false)
			{
				break;
			}
			pRow = pResult->fetch();
		}

		pResult->endQuery();
		delete pResult;

		for (std::map<std::string, STableField>::const_iterator it = table.m_mapField.begin();
			 it != table.m_mapField.end(); ++it)
		{
			STableField field = it->second;

			// deng: 如果表里没有这个字段，则增加这个字段；
			if (mapOldTable.find(field.m_strName) == mapOldTable.end())
			{
				std::stringstream ss;
				ss << "alter table " << table.m_strName << " add column "
				   << field.m_strName << " " << field.m_strType;

				LOG_INFO("在表中添加字段，表名= %s, 字段名 =  %s", table.m_strName, field.m_strName);
				std::string sql = ss.str();
				if (m_poConn->execute(sql.c_str()))
				{
					LOG_INFO("在表中添加字段成功！！！");
					// LOGI << sql;
					continue;
				}
				else
				{
					LOG_INFO("在表中添加字段 失败！！！");
					// LOGE << "CMysqlManager::_CheckTable failed : " << sql;
					return false;
				}
			}
		}
	}

	return true;
}

bool CMysqlManager::createTable(const STableInfo &table)
{
	if (table.m_mapField.size() == 0)
	{
		// LOGE << "CMysqlManager::_CreateTable, table info not valid, " << table.m_strName;
		return false;
	}

	std::stringstream ss;
	ss << "CREATE TABLE IF NOT EXISTS " << table.m_strName << " (";

	for (std::map<std::string, STableField>::const_iterator it = table.m_mapField.begin();
		 it != table.m_mapField.end(); ++it)
	{
		if (it != table.m_mapField.begin())
		{
			ss << ", ";
		}

		STableField field = it->second;
		ss << field.m_strName << " " << field.m_strType;
	}

	if (table.m_strKeyString != "")
	{
		ss << ", " << table.m_strKeyString;
	}

	ss << ")default charset = utf8, ENGINE = InnoDB;";
	if (m_poConn->execute(ss.str().c_str()))
	{
		return true;
	}

	LOGE("Create table error, sql: %s", ss.str().c_str());
	return false;
}
