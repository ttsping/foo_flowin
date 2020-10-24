#pragma once

// clang-format off

#define DECL_SHARED_PTR(t) using sp_t = std::shared_ptr<t>
#define DECL_WEAK_PTR(t) using wp_t = std::weak_ptr<t>
#define DECL_SMART_PTR(t) DECL_SHARED_PTR(t); DECL_WEAK_PTR(t)

// clang-format on
