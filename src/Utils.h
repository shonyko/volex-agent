#ifndef UTILS_H
#define UTILS_H

#define TEMPLATE_OF_TYPE(t)                                                    \
  template <class T,                                                           \
            typename std::enable_if_t<std::is_base_of_v<t, T>, bool> = true>

#define TEMPLATE_OF_TYPE_D(t, default_t)                                       \
  template <class T = default_t,                                               \
            typename std::enable_if_t<std::is_base_of_v<t, T>, bool> = true>

#endif