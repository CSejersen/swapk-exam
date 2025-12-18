#include "Job.hpp"

#include <exception>
namespace Factory {

    Job::Job(std::string name) : name_(name) {};

    void Job::addStep(JobStep step) {
        steps_.push(step);
    }

    const JobStep& Job::getNextStep() const {
        if(steps_.empty()) {
            throw std::out_of_range("no steps left in the job");
        }
        return steps_.front();
    }

    void Job::popStep() {
        steps_.pop();
    }

    bool Job::stepsEmpty() {
        return steps_.empty();
    }

    std::string Job::name() {
        return name_;
    }
}
