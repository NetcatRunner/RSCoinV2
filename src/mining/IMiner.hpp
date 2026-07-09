#pragma once

namespace RSCoin::Mining {

    class IMiner {
    public:
        virtual ~IMiner() = default;

        virtual void start() = 0;
        virtual void stop() = 0;
    };

}
