﻿
#include "SocketList.h"

namespace HttpServer
{
	SocketList::SocketList()
	{
	#ifdef WIN32
		this->obj_list = INVALID_HANDLE_VALUE;
	#elif POSIX
		this->obj_list = ~0;
	#else
		#error "Undefine platform"
	#endif
	}

	bool SocketList::create(const size_t startListSize)
	{
		destroy();

	#ifdef WIN32
		this->obj_list = (HANDLE) 1;

		if (startListSize > 0)
		{
			this->poll_events.reserve(startListSize);
		}

		return true;
	#elif POSIX
		this->obj_list = ::epoll_create(startListSize);

		if (this->obj_list == ~0)
		{
			return false;
		}

		if (startListSize > 0)
		{
			this->epoll_events.reserve(startListSize);
		}

		return true;
	#else
		#error "Undefine platform"
	#endif
	}

	void SocketList::destroy()
	{
		if (is_created() )
		{
		#ifdef WIN32
			this->obj_list = INVALID_HANDLE_VALUE;
			this->poll_events.clear();
		#elif POSIX
			::close(this->obj_list);
			this->obj_list = ~0;
			this->epoll_events.clear();
		#else
			#error "Undefine platform"
		#endif
		}
	}

	bool SocketList::is_created() const
	{
	#ifdef WIN32
		return this->obj_list != INVALID_HANDLE_VALUE;
	#elif POSIX
		return this->obj_list != ~0;
	#else
		#error "Undefine platform"
	#endif
	}

	bool SocketList::addSocket(const Socket &sock)
	{
		if (false == is_created() )
		{
			return false;
		}

	#ifdef WIN32
		WSAPOLLFD event = {
			sock.get_handle(),
			POLLRDNORM,
			0
		};

		poll_events.emplace_back(event);

		return true;
	#elif POSIX
		struct ::epoll_event event = {
			EPOLLIN | EPOLLET | EPOLLRDHUP,
			reinterpret_cast<void *>(sock.get_handle() )
		};

		const int result = ::epoll_ctl(this->obj_list, EPOLL_CTL_ADD, sock.get_handle(), &event);

		if (result == ~0)
		{
			return false;
		}

		this->epoll_events.emplace_back();

		return true;
	#else
		#error "Undefine platform"
	#endif
	}

	bool SocketList::removeSocket(const Socket &sock)
	{
		if (false == is_created() )
		{
			return false;
		}

	#ifdef WIN32
		for (size_t i = 0; i < poll_events.size(); ++i)
		{
			if (sock.get_handle() == poll_events[i].fd)
			{
				poll_events.erase(poll_events.begin() + i);
				return true;
			}
		}

		return false;
	#elif POSIX
		const int result = ::epoll_ctl(this->obj_list, EPOLL_CTL_DEL, sock.get_handle(), nullptr);

		if (result == ~0)
		{
			return false;
		}

		this->epoll_events.pop_back();

		return true;
	#else
		#error "Undefine platform"
	#endif
	}

	bool SocketList::accept(std::vector<Socket> &sockets) const
	{
		if (is_created() )
		{
		#ifdef WIN32
			const int count = ::WSAPoll(this->poll_events.data(), this->poll_events.size(), ~0);

			if (SOCKET_ERROR == count)
			{
				return false;
			}

			for (size_t i = 0; i < this->poll_events.size(); ++i)
			{
				const WSAPOLLFD &event = this->poll_events[i];

				if (event.revents & POLLRDNORM)
				{
					System::native_socket_type client_socket = ~0;

					do
					{
						client_socket = ::accept(event.fd, static_cast<sockaddr *>(nullptr), static_cast<int *>(nullptr) );

						if (~0 != client_socket)
						{
							sockets.emplace_back(Socket(client_socket) );
						}
					}
					while (~0 != client_socket);
				}
			}

			return false == sockets.empty();
		#elif POSIX
			const int count = ::epoll_wait(this->obj_list, this->epoll_events.data(), this->epoll_events.size(), ~0);

			if (count == ~0)
			{
				return false;
			}

			for (int i = 0; i < count; ++i)
			{
				const epoll_event &event = this->epoll_events[i];

				if (event.events & EPOLLIN)
				{
					System::native_socket_type client_socket = ~0;

					do
					{
						client_socket = ::accept(event.data.fd, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr) );

						if (~0 != client_socket)
						{
							sockets.emplace_back(Socket(client_socket) );
						}
					}
					while (~0 != client_socket);
				}
			}

			return false == sockets.empty();
		#else
			#error "Undefine platform"
		#endif
		}

		return false;
	}

	bool SocketList::accept(std::vector<Socket> &sockets, std::vector<struct sockaddr_in> &socketsAddress) const
	{
		if (is_created() )
		{
		#ifdef WIN32
			const int count = ::WSAPoll(this->poll_events.data(), this->poll_events.size(), ~0);

			if (SOCKET_ERROR == count)
			{
				return false;
			}

			for (size_t i = 0; i < this->poll_events.size(); ++i)
			{
				const WSAPOLLFD &event = this->poll_events[i];

				if (event.revents & POLLRDNORM)
				{
					System::native_socket_type client_socket = ~0;

					do
					{
						struct ::sockaddr_in client_addr = {};
						socklen_t client_addr_len = sizeof(client_addr);

						client_socket = ::accept(event.fd, reinterpret_cast<struct ::sockaddr *>(&client_addr), &client_addr_len);

						if (~0 != client_socket)
						{
							sockets.emplace_back(Socket(client_socket) );
							socketsAddress.emplace_back(client_addr);
						}
					}
					while (~0 != client_socket);
				}
			}

			return false == sockets.empty();
		#elif POSIX
			const int count = ::epoll_wait(this->obj_list, this->epoll_events.data(), this->epoll_events.size(), ~0);

			if (count == ~0)
			{
				return false;
			}

			for (int i = 0; i < count; ++i)
			{
				const epoll_event &event = this->epoll_events[i];

				if (event.events & EPOLLIN)
				{
					System::native_socket_type client_socket = ~0;

					do
					{
						struct ::sockaddr_in client_addr = {};
						socklen_t client_addr_len = sizeof(client_addr);

						client_socket = ::accept(event.data.fd, reinterpret_cast<struct ::sockaddr *>(&client_addr), &client_addr_len);

						if (~0 != client_socket)
						{
							sockets.emplace_back(Socket(client_socket) );
							socketsAddress.emplace_back(client_addr);
						}
					}
					while (~0 != client_socket);
				}
			}

			return false == sockets.empty();
		#else
			#error "Undefine platform"
		#endif
		}

		return false;
	}

	bool SocketList::recv(std::vector<Socket> &sockets, std::vector<Socket> &disconnected, std::chrono::milliseconds timeout) const
	{
		if (false == is_created() )
		{
			return false;
		}

	#ifdef WIN32
		const int count = ::WSAPoll(this->poll_events.data(), this->poll_events.size(), timeout.count() );

		if (SOCKET_ERROR == count)
		{
			return false;
		}

		for (size_t i = 0; i < this->poll_events.size(); ++i)
		{
			const WSAPOLLFD &event = this->poll_events[i];

			if (event.revents & POLLRDNORM)
			{
				sockets.emplace_back(Socket(event.fd) );
			}
			else if (event.revents & POLLHUP)
			{
				disconnected.emplace_back(Socket(event.fd) );
			}
		}

		return false == sockets.empty() || false == disconnected.empty();
	#elif POSIX
		const int count = ::epoll_wait(this->obj_list, this->epoll_events.data(), this->epoll_events.size(), timeout.count() );

		if (count == ~0)
		{
			return false;
		}

		for (int i = 0; i < count; ++i)
		{
			const epoll_event &event = this->epoll_events[i];

			if (event.events & EPOLLIN)
			{
				sockets.emplace_back(Socket(event.data.fd) );
			}
			else if (event.events & EPOLLRDHUP)
			{
				disconnected.emplace_back(Socket(event.data.fd) );
			}
		}

		return false == sockets.empty() || false == disconnected.empty();
	#else
		#error "Undefine platform"
	#endif
	}
};