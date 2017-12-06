#pragma once

class BaseExecutor;

class Job
{
	void* worker;
	void(*job)(void*const worker, BaseExecutor* const executor);
public:
	Job(void* const worker, void(*job)(void*const worker, BaseExecutor* const executor)) : worker(worker), job(job) {}
	Job() {}

	void operator()(BaseExecutor* const executor)
	{
		job(worker, executor);
	}

	Job operator=(const Job other)
	{
		this->job = other.job;
		this->worker = other.worker;
		return *this;
	}


	bool operator==(const Job other)
	{
		return this->worker == other.worker && this->job == other.job;
	}

	bool operator!=(const Job other)
	{
		return this->worker != other.worker || this->job != other.job;
	}
};