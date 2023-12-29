#pragma once
#include <functional>
#include <utility>
#include <memory>

#include <glaze/glaze.hpp>

#include "mech_suit/meta_string.hpp"
#include "mech_suit/body.hpp"
#include "mech_suit/path_params.hpp"
#include "mech_suit/http_request.hpp"
#include "mech_suit/http_response.hpp"
#include "mech_suit/route.hpp"

namespace mech_suit {

    class application {
    private:
        std::unordered_map<std::string_view, std::unique_ptr<detail::base_route>> m_routes;
        std::vector<std::unique_ptr<detail::base_route>> m_dynamic_routes;

        template <meta::string Path, http_method Method, typename Body=void>
        void add_route(detail::callback_type_t<Path, Method, Body> cb)
        {
            if constexpr (0 == std::tuple_size<typename detail::params_tuple<Path>::type>()) {
                m_routes.emplace(Path, std::make_unique<detail::route<Path, Method, Body>>(cb));
            } else {
                m_dynamic_routes.emplace_back(std::make_unique<detail::route<Path, Method, Body>>(cb));
            }
        }

        void handle_request(const http_request& request, http_response& response)
        {
            std::string path; //todo

            if (m_routes.count(path)) {
                if (not m_routes[path]->process(request, response)) [[unlikely]] {
                    // route not handled
                    // this should ONLY happen due to programming error, since the path is not dynamic
                    throw std::runtime_error("Explicit route refused to handle a request");
                }
            } else {
                for (const auto& route: m_dynamic_routes) {
                    if (route->process(request, response)) {
                        return;
                    }
                }
            }
        }

    public:
        template <meta::string Path>
        void get(detail::callback_type_t<Path, GET> cb)
        {
            add_route<Path, GET>(cb);
        }

        template <meta::string Path, typename Body=void>
        void post(detail::callback_type_t<Path, POST, Body> cb)
        {
            add_route<Path, POST, Body>(cb);

            /* http_request request; */
            /* if constexpr (body_is_json_v<Body>) { */
            /*     using type_t = typename Body::type; */
            /*     type_t body; */
            /*     auto err = glz::read_json<type_t>(body, request.body_raw); */
            /*     if (err) { */
            /*         std::string descriptive_error = glz::format_error(err, request.body_raw); */
            /*         // do something with error */
            /*     } else { */
            /*         // do something with body */
            /*     } */
            /* } */
        }
    };
};

