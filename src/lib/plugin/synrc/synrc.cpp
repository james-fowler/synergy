/*
 * Copyright (C) 2015 James Fowler
 */

#include "../synrc/synrc.h"

#include "base/IJob.h"
#include "base/Log.h"
#include "arch/Arch.h"
#include "mt/Thread.h"
#include "arch/Arch.h"
#include "server/Server.h"
#include "server/ServerPluginCommand.h"
#include "server/ServerPluginFeedback.h"
#include "synergy/ServerApp.h"
#include "synergy/ServerArgs.h"
#include "synergy/IClipboardAccess.h"

#include "common/PluginVersion.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

/*!
 * simple wrapper for named pipes
 */
class NamedPipe {
public:
	std::string _name;
	int _fd;

	NamedPipe() : _fd(-1)
	{
	}

	bool isopen() const { return _fd > 0; }
	void open( const std::string& name ) {
		if( isopen() ) close();

		_name = name;
		mkfifo(_name.c_str(), 0666);
		chmod( _name.c_str(), 0777 );
		_fd = ::open( _name.c_str(), O_RDWR | O_NONBLOCK);
	}

	void close() {
		if( _fd > 0 ) {
			::close( _fd );
		}
		remove( _name.c_str() );
		_fd = -1;
	}

	ssize_t write( const std::string &text ) {
		ssize_t rv;
		rv = ::write(_fd, text.c_str(), text.size() );
		return rv;
	}

	int read(char *buf, size_t max) {

		fd_set set;
		FD_ZERO(&set);
		FD_SET(_fd, &set);
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		int ret = select(_fd + 1, &set, 0, 0, &tv);
		if (ret > 0) {
			memset(buf, 0, max);
			ret = ::read(_fd, buf, max);
		}
		return ret;
	}
};


class SynRcJob : public IJob {
public:

	SynRcJob( void *l, void *a ) :
		log(reinterpret_cast<Log*>(l)),
		arch(reinterpret_cast<Arch*>(a)),
		sendEvent(0),
		s_running(false),
		pipe_name( "/tmp/SynRc"),
		my_thread(0)
	{
		assert( log != 0 );
		assert( arch != 0 );
	}

	Log *log;
	Arch *arch;
	void (*sendEvent)(const char*, void*);
	bool s_running;
	std::string pipe_name;
	Thread *my_thread;
	NamedPipe input_pipe;
	NamedPipe output_pipe;
	Server *m_server;
	PluginFeedbackPtr m_pluginFeedback;

	SPFC_String::CallbackEntry m_switched_callback;

	int initEvent(void (*arg_sendEvent)(const char*, void*))
	{
		assert( arg_sendEvent != 0 );
		sendEvent = arg_sendEvent;

		m_server = ServerApp::instance().getServerPtr();
		assert( m_server != 0 );

		m_pluginFeedback = m_server->pluginFeedback();
		assert( m_pluginFeedback.get() != 0 );
		LOG((CLOG_NOTE "m_pluginFeedback = %p", (void*)m_pluginFeedback.get() ));



		input_pipe.open( pipe_name );
		my_thread = new Thread( this );
		m_switched_callback = m_pluginFeedback->screenSwitched().add( &SynRcJob::handle_screen_switched, this );

	}

	void handle_screen_switched( const std::string &screen ) {

		LOG((CLOG_NOTE "SynRc handle_screen_switched %s", screen.c_str() ));
		if( output_pipe.isopen() ) {
			output_pipe.write( "switchedToScreen " + screen + "\n" );
		}
	}

	//! Run the job
	virtual void		run() {

		s_running = true;
		LOG((CLOG_NOTE "starting SynRc plugin"));

		while (s_running) {
			check_pipe();
			arch->sleep(0.0);
		}

		LOG((CLOG_NOTE "exiting SynRc plugin"));
	}

	void handle_single_command( const std::string &command ) {
		LOG((CLOG_NOTE  "handle_single_command: %s",command.c_str() ));
		std::vector<std::string> tokens;
		boost::split(tokens, command, boost::is_any_of(" \t"));

		//if( tokens[0] == "switchToScreen" ) {
		//	sendSwitchScreen( tokens[1] );
		//} else
		if( tokens[0] == "openNotifyPipe" ) {
			output_pipe.open( tokens[1] );
		} else {
			LOG((CLOG_DEBUG "creating ServerPluginCommand" ));

			ServerPluginCommandHandle cmd = ServerPluginCommand::create( tokens );
			LOG((CLOG_INFO "submitting ServerPluginCommand" ));
			ServerApp::instance().getServerPtr()->submitPluginCommand( cmd );
			// write_pipe_line( "unknown command : " + command );
		}
		LOG((CLOG_NOTE  "handle_single_command complete: %s",command.c_str() ));
	}

	void handle_command_text(const std::string &input) {
		std::vector<std::string> lines;
		boost::split(lines, input, boost::is_any_of("\n\r"));

		BOOST_FOREACH(std::string const &cmd, lines) {
			std::vector<std::string> command;
			boost::split(command, cmd, boost::is_any_of("#"));
			if (command.size() > 0 && command[0] != std::string("")) {
				handle_single_command( command[0] );
			}
		}
	}

	void write_pipe_line( const std::string &text ) {
		if( output_pipe.isopen() ) {
			LOG((CLOG_NOTE  "SynRc WPL writing: %s", text.c_str() ));
			output_pipe.write( text + "\n" );
		} else {
			LOG((CLOG_NOTE  "SynRc WPL (output pipe not open): %s", text.c_str() ));
		}
	}

	void check_pipe() {
		const size_t BUF_SIZE = 1024;
		char buf[BUF_SIZE];
		int ret = input_pipe.read( buf,  BUF_SIZE );

		if (ret > 0) {
			assert( ret < BUF_SIZE );
			buf[ ret ] = 0;
			handle_command_text(buf);
		}
	}

	void* invoke(const char* command, void** args)
	{
		IEventQueue* arg1 = NULL;

		if (args != NULL) {
			arg1 = reinterpret_cast<IEventQueue*>(args[0]);
		}

		else if (strcmp(command, "version") == 0) {
			return (void*)getExpectedPluginVersion(s_pluginNames[kSyncRc]);
		}

		return NULL;
	}

	void
	cleanup()
	{
		m_pluginFeedback->screenSwitched().remove( m_switched_callback );
		s_running = false;
		input_pipe.close();
		m_pluginFeedback.reset();

	}

	static SynRcJob *my_job;
};

SynRcJob *SynRcJob::my_job;

extern "C" {

void
init(void* log, void* arch)
{
	LOG((CLOG_NOTE  "SynRc init" ));
	SynRcJob::my_job = new SynRcJob( log, arch );
}

int
initEvent(void (*sendEvent)(const char*, void*))
{
	LOG((CLOG_NOTE  "SynRc initEvent" ));
	assert( SynRcJob::my_job != 0 );
	return SynRcJob::my_job-> initEvent( sendEvent );
}

void*
invoke(const char* command, void** args)
{
	LOG((CLOG_NOTE  "SynRc invoke \"%s\"", command ));
	if( SynRcJob::my_job == 0 && !strcmp(command, "version") ) {
		return (void*)getExpectedPluginVersion(s_pluginNames[kSyncRc]);
	}
	assert( SynRcJob::my_job != 0 );
	return SynRcJob::my_job->invoke( command, args );
}

void
cleanup()
{
	assert( SynRcJob::my_job != 0 );
	SynRcJob::my_job->cleanup();
}

} // extern "C"void
