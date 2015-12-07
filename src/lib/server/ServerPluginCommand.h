/*
 * ServerPluginCommand.h
 *
 * Copyright (C) 2015 James Fowler
 *
 *  Created on: Dec 6, 2015
 *      Author: jfowler
 */

#ifndef SRC_LIB_SERVER_SERVERPLUGINCOMMAND_H_
#define SRC_LIB_SERVER_SERVERPLUGINCOMMAND_H_

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include "base/Log.h"

#if 1
#define SPDB_LOG0( text ) 					LOG((CLOG_DEBUG text ))
#define SPDB_LOG1( text, a1 )			 	LOG((CLOG_DEBUG text, a1 ))
#else
#define SPDB_LOG0( text )
#define SPDB_LOG1( text, a1 )
#endif

/*!
 * holds a command to be sent to the server
 */
class ServerPluginCommand {
public:
	ServerPluginCommand() : m_result_posted(false) {
		SPDB_LOG1( "new ServerPluginCommandImpl at %p", this );
	}
	virtual ~ServerPluginCommand() {
		SPDB_LOG1( "del ServerPluginCommandImpl at %p", this );
	};

	std::string m_cmd;
	std::vector<std::string> m_args;

	boost::function< void (const std::string &) > m_post_result;

	void fail( const std::string &result ) {
		if( m_result_posted ) {
			LOG((CLOG_ERR "multiple result for command %s : %s", m_cmd.c_str(), result.c_str() ));;
			return;
		}
		m_result_posted = true;
		LOG((CLOG_ERR "Plugin Command %s failed : %s", m_cmd.c_str(), result.c_str() ));;
	}

	void respond( const std::string &result ) {
		if( m_result_posted ) {
			LOG((CLOG_ERR "multiple result for command %s : %s", m_cmd.c_str(), result.c_str() ));;
			return;
		}
		m_result_posted = true;
		LOG((CLOG_ERR "Plugin Command %s response : %s", m_cmd.c_str(), result.c_str() ));;

	}

	static ServerPluginCommandHandle create( const std::vector<std::string> &tokens) {
		ServerPluginCommandHandle impl( new ServerPluginCommand() );

		if( tokens.size() ) impl->m_cmd = tokens[0];
		if( tokens.size() > 1 ) {
			for( size_t x = 1; x < tokens.size(); x++ ) {
				impl->m_args.push_back( tokens[x] );
			}
		}
		return impl;
	}
protected:
	bool m_result_posted;
};

class ServerPluginException : public std::exception {

public:
	const char *	m_file;
	int 			m_line;
	std::string 	m_message;

	ServerPluginException( const char *file, int line, const std::string &message ) : m_file(file), m_line(line), m_message(message)
	{}
	~ServerPluginException() throw () {}

	virtual const char* what() const throw () { return m_message.c_str(); }

};

#define SPC_ENSUREM( condition, message )	\
	if( !(condition) ) { throw ServerPluginException( __FILE__, __LINE__, message ); }

/*!
 This class is a wrapper used to transfer a ServerPluginCommand through the event queue
*/
class ServerPluginCommandData : public EventData {
public:
	ServerPluginCommandData( const ServerPluginCommandHandle & impl ) : _impl(impl) {}

	ServerPluginCommandHandle impl() { return _impl; }

protected:
	friend class ServerPluginCommand;
	ServerPluginCommandHandle _impl;
};


#endif /* SRC_LIB_SERVER_SERVERPLUGINCOMMAND_H_ */
