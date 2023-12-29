#pragma once

#include "mech_suit/path_params.hpp"
#include "mech_suit/http_request.hpp"
#include "mech_suit/http_response.hpp"
#include "mech_suit/http_method.hpp"

namespace mech_suit::detail {
    template<http_method Method, typename T, typename Body>
    struct route_callback;

    template<http_method Method, typename... Ts>
    struct route_callback<Method, std::tuple<Ts...>, void> {
        using type = std::function<void(const http_request&, http_response&, const typename Ts::type&...)>;
    };

    template<http_method Method, typename... Ts, typename Body>
    struct route_callback<Method, std::tuple<Ts...>, Body> {
        using type = std::function<void(const http_request&, http_response&, const typename Ts::type&..., const typename Body::type&)>;
    };

    template<meta::string Path, http_method Method, typename Body>
    struct callback_type {
        using params_tuple_t = typename params_tuple<Path>::type;
        using type = route_callback<Method, params_tuple_t, Body>::type;
    };

    template<meta::string Path, http_method Method, typename Body=void> using callback_type_t = callback_type<Path, Method, Body>::type;

    class base_route {
    private:
        std::string_view m_path;

    protected:
        virtual void handle_request(const http_request& request, http_response& response) = 0;
    public:
        base_route(std::string_view path) : m_path(path) {}
        virtual ~base_route() = default;

        bool process(const http_request& request, http_response& response)
        {
            // TODO:
            // if path matches, handle_request
            (void) request;
            (void) response;
            return false;
        }
    };

    template<meta::string Path, http_method Method, typename Body=void>
    class route : public base_route {
    public:
        static constexpr std::string_view path = Path;
        using callback_t = callback_type_t<Path, Method, Body>;

        route(callback_t cb)
            : base_route(path)
            , callback(cb)
        {}

    protected:
        void handle_request(const http_request& request, http_response& response) override
        {
            // TODO
            (void) request;
            (void) response;
        }

    private:
        callback_t callback;
    };
}

