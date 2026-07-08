#ifndef JOB_SYSTEM_HPP
#define JOB_SYSTEM_HPP

namespace Application {
    class JobSystem {
        public:
            static JobSystem& getInstance();

            JobSystem(const JobSystem&) = delete;
            JobSystem(JobSystem&&) noexcept = delete;

            ~JobSystem();

            JobSystem& operator=(const JobSystem&) = delete;
            JobSystem& operator=(JobSystem&&) noexcept = delete;

        private:
            JobSystem();
    };
}

#endif