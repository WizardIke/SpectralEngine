#pragma once
class BaseExecutor;
class SharedResources;

class Job
{
	using Function = void(*)(void*const requester, BaseExecutor* const executor, SharedResources& sharedResources);

	void* requester;
	Function job;
public:
	Job(void* const requester, Function job) : requester(requester), job(job) {}
	Job() {}

	void operator()(BaseExecutor* const executor, SharedResources& sharedResources)
	{
		job(requester, executor, sharedResources);
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