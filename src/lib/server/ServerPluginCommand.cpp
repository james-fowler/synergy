/*
 *
 * Copyright (C) 2015 James Fowler
 * 
 */

#include "server/Server.h"

#include "base/Log.h"
#include "common/stdexcept.h"
#include "server/ServerPluginCommand.h"
#include "server/ServerPluginFeedback.h"
#include "server/ClientProxy.h"
#include "synergy/IClipboardAccess.h"

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <fstream>


#define FEEDBACK_CHANNEL_INIT( NAME )  m_##NAME( *this, #NAME )

ServerPluginFeedback::ServerPluginFeedback( Server *s ) :
	FEEDBACK_CHANNEL_INIT( screenSwitched ),
	m_server(s)
{
}


void Server::initPluginFeedback() {
	m_pluginFeedback = PluginFeedbackPtr( new ServerPluginFeedback(this) );
}


void Server::submitPluginCommand( const ServerPluginCommandHandle &cmd ) {
	/**
	 * The initEvent / sendEvent system appears to be limited to event types
	 * handled by adoptHandler( ..., m_primaryClient->getEventTarget(), ... ).
	 * So, to avoid the need for many flavors of sendEvent or for plugins to
	 * find the correct target / queue and manually create events, this
	 * mechanism allows commands to be sent to the server from a plugin
	 * simply and cleanly, even from other threads.
	 *
	 */

	ServerPluginCommandData *cmd_wrapper = new ServerPluginCommandData( cmd );
	SPDB_LOG1( "submitPluginCommand  cmd_wrapper at %p", cmd_wrapper );
	Event e( m_events->forServer().pluginCommand(), this );
	e.setDataObject( cmd_wrapper );
	m_events->addEvent(e);


}


void Server::handlePluginCommandEvent(const Event&e, void*) {

	ServerPluginCommandData *cmd_wrapper = reinterpret_cast<ServerPluginCommandData *>(e.getDataObject());

	SPDB_LOG1( "handlePluginCommandEvent  cmd_wrapper at %p", cmd_wrapper );


	ServerPluginCommand &i ( *(cmd_wrapper->impl()) );

	LOG((CLOG_NOTE "Server::handlePluginCommandEvent %s", i.m_cmd.c_str() ));
	try {
		if( i.m_cmd == "switchToScreen" ) {
			SPC_ENSUREM( i.m_args.size() == 1, "wrong argument count" );
			std::string screen_name = i.m_args[0];
			ClientList::const_iterator index = m_clients.find(screen_name);
			SPC_ENSUREM(index != m_clients.end(), "screen not found" );
			jumpToScreen(index->second);
			return;
		}

		if( i.m_cmd == "moveXY" ) {
			SPC_ENSUREM( i.m_args.size() == 2, "wrong argument count" );
			SInt32 x = boost::lexical_cast<SInt32>( i.m_args[0] );
			SInt32 y = boost::lexical_cast<SInt32>( i.m_args[1] );
			LOG((CLOG_NOTE "mouseMove( %d, %d ) at %p(%s)", (int)x, (int)y, m_active, m_active->getName().c_str() ));

			mouseMove( x, y );

			return;
		}

		if( i.m_cmd == "dump_clipboards" ) {
			LOG((CLOG_NOTE "dumping clipboards"));
			IClipboardAccess::dump_clipboards();
			return;
		}
		i.fail( "unknown command : " + i.m_cmd );
	}
	catch (const std::exception &ex ) {
		i.fail( ex.what() );
	}
}


