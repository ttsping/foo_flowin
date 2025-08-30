#pragma once

#define RETURN_VOID_IF(condition)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((condition))                                                                                               \
            return;                                                                                                    \
    } while (false)

#define RETURN_HR_IF(hr, condition)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((condition))                                                                                               \
            return (hr);                                                                                               \
    } while (false)
