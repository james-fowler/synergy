/*
 * ServerPluginFeedback.h
 *
 * Copyright (C) 2015 James Fowler
 *
 *  Created on: Dec 6, 2015
 *      Author: jfowler
 */

#ifndef SRC_LIB_SERVER_SERVERPLUGINFEEDBACK_H_
#define SRC_LIB_SERVER_SERVERPLUGINFEEDBACK_H_

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/any.hpp>

#include "base/Log.h"


enum PluginFeedbackPoints {
	PF_SCREEN_SWITCHED,
	PF_LAST
};

class ServerPluginFeedback;

template <class T>
class ServerPluginFeedbackChannel {
public:
	friend class ServerPluginFeedback;

	ServerPluginFeedbackChannel(ServerPluginFeedback&parent, const std::string &name) : m_parent(parent), m_name(name), _next_id(0) {}

	typedef boost::function< void (const T &) > Callback;

	struct CallbackEntry {
		int 			m_id;
		Callback 		m_f;

		CallbackEntry() : m_id(-1) {}
		CallbackEntry( const Callback &f, int i=-1) : m_id(i), m_f(f) {}

		bool operator == (const CallbackEntry &other) const { return m_id == other.m_id; }
	};


	void remove( const CallbackEntry &cb ) {
		_callbacks.erase( std::remove(_callbacks.begin(), _callbacks.end(), cb), _callbacks.end());
	}

	template <class F, class C>
	CallbackEntry add( F f, C *c ) {
		return addEntry( boost::bind( f, c, _1 ) );
	};


	void invoke( const T &val ) {

		for( size_t i = 0; i < _callbacks.size(); i++ ) {
			try {
				_callbacks[i].m_f(val);
			}
			catch (const std::exception &ex) {
				LOG((CLOG_NOTE "plugin feedback invoke channel %s exception : ", m_name.c_str(), ex.what() ));
			}
			catch (...) {

			}
		}
	}

protected:
	CallbackEntry addEntry( const Callback &f ) {
		int id = _next_id++;
		CallbackEntry e(f, id);
		_callbacks.push_back(e);
		return e;
	}

	ServerPluginFeedback & 			m_parent;
	std::string						m_name;
	int								_next_id;
	std::vector<CallbackEntry> 		_callbacks;

};

typedef ServerPluginFeedbackChannel<std::string> SPFC_String;

class ServerPluginFeedback {
public:
	ServerPluginFeedback( Server *s );

	void				sendFeedback(PluginFeedbackPoints);
	void				sendFeedback(PluginFeedbackPoints, boost::any);



#define FEEDBACK_CHANNEL( TYPE, NAME )										\
	protected:																\
		ServerPluginFeedbackChannel<TYPE> m_##NAME;							\
	public:																	\
		ServerPluginFeedbackChannel<TYPE> &NAME() { return m_##NAME; }		\
		const ServerPluginFeedbackChannel<TYPE> &NAME() const 				\
													{ return m_##NAME; }	\


	FEEDBACK_CHANNEL( std::string, screenSwitched )


protected:

	Server *		m_server;
};

#endif /* SRC_LIB_SERVER_SERVERPLUGINFEEDBACK_H_ */
