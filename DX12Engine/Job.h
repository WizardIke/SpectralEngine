#pragma once

class BaseExecutor;

class Job
{
	void* requester;
	void(*job)(void*const requester, BaseExecutor* const executor);
public:
	Job(void* const requester, void(*job)(void*const requester, BaseExecutor* const executor)) : requester(requester), job(job) {}
	Job() {}

	void operator()(BaseExecutor* const executor)
	{
		job(requester, executor);
	}

	void operator=(const Job& other)
	{
		job = other.job;
		requester = other.requester;
	}


	bool operator==(const Job other)
	{
		return requester == other.requester && job == other.job;
	}

	bool operator!=(const Job other)
	{
		return requester != other.requester || job != other.job;
	}
};