#ifndef STRIPED_SYNC_HPP
#define STRIPED_SYNC_HPP
#include <vector>
#include <algorithm>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/noncopyable.hpp>
#include <boost/array.hpp>

template <typename T, int size = 8>
class striped_sync : public boost::noncopyable{
public:
	
	class ref : public boost::noncopyable{
	public:
		ref(striped_sync<T,size>& obj, int hash):m_hash(hash),m_ref(&obj)
		{
			obj.m_lock[hash%size].lock();
		}			
		~ref() { reset(); }
		void reset()
		{
			if(m_ref) {
				m_ref->m_lock[m_hash%size].unlock();
				m_ref = NULL;
			}
		}
		T& operator*() { return m_ref->m_obj; }
		T* operator->() { return &operator*(); }
		const T& operator*() const { return m_ref->m_obj; }
		const T* operator->() const { return &operator*(); }
		boost::mutex& get_mutex(){ return m_ref->m_mutex;}
	private:
		int m_hash;
		striped_sync<T,size>* m_ref;
	};

	class multi_ref : public boost::noncopyable{
		multi_ref(striped_sync<T,size>& obj, const std::vector<int>& hash)
			:m_hash(hash),m_ref(&obj)
		{
			std::sort(m_hash.begin(),m_hash.end());
			m_hash.erase(std::unique(m_hash.begin(),m_hash.end()),m_hash.end());
			for(int i=0;i<m_hash.size();i++){
				obj.m_lock[m_hash[i]%size].lock();
			}
		}
		~multi_ref(){
			for(int i=m_hash.size()-1;i>=0;--i){
				m_ref->m_lock[m_hash[i]%size].unlock();
			}
			m_ref = NULL;
		}
		T& operator*() { return m_ref->m_obj; }
		T* operator->() { return &operator*(); }
		const T& operator*() const { return m_ref->m_obj; }
		const T* operator->() const { return &operator*(); }
		boost::mutex& get_mutex(){ return m_ref->m_mutex;}
	private:
		std::vector<int> m_hash;
		striped_sync<T,size>* m_ref;
	};
	
private:
	T m_obj;
	boost::array<boost::mutex, size> m_lock;
	friend class ref;
};


template <typename T, int size>
class striped_rwsync : public boost::noncopyable{
public:
	class wref;
	class rref : public boost::noncopyable{
	public:
		rref(striped_rwsync<T,size>& obj, int hash)
			:m_hash(hash%size),lock(obj.m_lock[m_hash]),m_ref(&obj) {}			
		~rref() {}
		T& operator*() { return m_ref->m_obj; }
		T* operator->() { return &operator*(); }
		const T& operator*() const { return m_ref->m_obj; }
		const T* operator->() const { return &operator*(); }
		boost::mutex& get_mutex(){ return m_ref->m_mutex;}
	private:
		int m_hash;
		boost::shared_lock<boost::shared_mutex> lock;
		striped_rwsync<T,size>* m_ref;
		friend class wref;
	};
	class wref : public boost::noncopyable{
	public:
		wref(rref& obj)
		:m_ref(obj.m_ref),lock(m_ref->m_lock[obj.m_hash]) {}
		~wref() { reset(); }
		void reset()
		{}
		bool owns_lock()const{return lock.owns_lock();}
		T& operator*() { return m_ref->m_ref->m_obj; }
		T* operator->() { return &operator*(); }
		const T& operator*() const { return m_ref->m_ref->m_obj; }
		const T* operator->() const { return &operator*(); }
	private:
		striped_rwsync<T,size>* m_ref;
		boost::upgrade_lock<boost::shared_mutex> lock;
	};

	class multi_rref : public boost::noncopyable{
		multi_rref(striped_rwsync<T,size>& obj, const std::vector<int>& hash)
			:m_hash(hash),m_ref(&obj)
		{
			std::sort(m_hash.begin(),m_hash.end());
			m_hash.erase(std::unique(m_hash.begin(),m_hash.end()),m_hash.end());
			for(int i=0;i<m_hash.size();i++){
				obj.m_lock[m_hash[i]%size].lock();
			}
		}
		~multi_rref(){
			for(int i=m_hash.size()-1;i>=0;--i){
				m_ref->m_lock[m_hash[i]%size].unlock();
			}
			m_ref = NULL;
		}
		T& operator*() { return m_ref->m_obj; }
		T* operator->() { return &operator*(); }
		const T& operator*() const { return m_ref->m_obj; }
		const T* operator->() const { return &operator*(); }
		boost::mutex& get_mutex(){ return m_ref->m_mutex;}
	private:
		std::vector<int> m_hash;
		striped_rwsync<T,size>* m_ref;
	};
	
private:
	T m_obj;
	boost::array<boost::shared_mutex, size> m_lock;
	friend class rref;
	friend class wref;
};
#endif
