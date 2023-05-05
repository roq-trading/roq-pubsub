/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/bybit/json/order_book.hpp"
#include "roq/bybit/json/parser.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto const MESSAGE_1 = R"({)"
                       R"("topic":"orderbook.1.BTCUSDT",)"
                       R"("ts":1682083233502,)"
                       R"("type":"delta",)"
                       R"("data":{)"
                       R"("s":"BTCUSDT",)"
                       R"("b":[)"
                       R"(["27579.99","0.030954"])"
                       R"(],)"
                       R"("a":[],)"
                       R"("u":866051,)"
                       R"("seq":792472862)"
                       R"(})"
                       R"(})"sv;
auto const MESSAGE_50_SNAPSHOT = R"({)"
                                 R"("topic":"orderbook.50.BTCUSDT",)"
                                 R"("ts":1682083230023,)"
                                 R"("type":"snapshot",)"
                                 R"("data":{)"
                                 R"("s":"BTCUSDT",)"
                                 R"("b":[)"
                                 R"(["27579.99","0.031443"],)"
                                 R"(["27579.97","0.085285"],)"
                                 R"(["27579.95","0.084408"],)"
                                 R"(["27579.93","0.001952"],)"
                                 R"(["27579.91","0.039404"],)"
                                 R"(["27579.89","0.07013"],)"
                                 R"(["27579.87","0.028235"],)"
                                 R"(["27579.85","0.023555"],)"
                                 R"(["27579.83","0.038158"],)"
                                 R"(["27579.81","0.047517"],)"
                                 R"(["27494.73","0.00471"],)"
                                 R"(["27478.41","0.001163"],)"
                                 R"(["27458.9","0.002498"],)"
                                 R"(["27454.5","0.00264"],)"
                                 R"(["27450","0.003"],)"
                                 R"(["27446.4","0.001714"],)"
                                 R"(["27438.55","0.000719"],)"
                                 R"(["27436.83","0.001"],)"
                                 R"(["27432.94","0.001013"],)"
                                 R"(["27427.3","0.000687"],)"
                                 R"(["27413.6","0.000593"],)"
                                 R"(["27410.52","0.00471"],)"
                                 R"(["27405.88","0.001006"],)"
                                 R"(["27402.2","0.004378"],)"
                                 R"(["27400","0.004439"],)"
                                 R"(["27399.69","0.000555"],)"
                                 R"(["27399.52","0.004137"],)"
                                 R"(["27399.47","0.000313"],)"
                                 R"(["27397.56","0.001302"],)"
                                 R"(["27392.85","0.019715"],)"
                                 R"(["27392","0.030462"],)"
                                 R"(["27379.23","0.000544"],)"
                                 R"(["27376.74","0.002177"],)"
                                 R"(["27375","0.007904"],)"
                                 R"(["27363.62","0.011344"],)"
                                 R"(["27363.6","0.00264"],)"
                                 R"(["27357.86","0.000281"],)"
                                 R"(["27357.27","0.000424"],)"
                                 R"(["27356.52","0.000521"],)"
                                 R"(["27356.2","0.001714"],)"
                                 R"(["27352.24","0.000182"],)"
                                 R"(["27350.8","0.002498"],)"
                                 R"(["27350","0.003"],)"
                                 R"(["27347.75","0.000867"],)"
                                 R"(["27343.84","0.001"],)"
                                 R"(["27342.56","0.0002"],)"
                                 R"(["27340","0.000927"],)"
                                 R"(["27338.8","0.000558"],)"
                                 R"(["27336.15","0.001013"],)"
                                 R"(["27333.32","0.000341"])"
                                 R"(],)"
                                 R"("a":[)"
                                 R"(["27580","0.080386"],)"
                                 R"(["27580.14","0.038662"],)"
                                 R"(["27580.76","0.000186"],)"
                                 R"(["27580.8","0.000495"],)"
                                 R"(["27584","0.030431"],)"
                                 R"(["27586.36","0.001004"],)"
                                 R"(["27587.39","0.000689"],)"
                                 R"(["27588.52","0.026599"],)"
                                 R"(["27592.04","0.000303"],)"
                                 R"(["27600","0.030491"],)"
                                 R"(["27605.49","0.000139"],)"
                                 R"(["27607","0.014594"],)"
                                 R"(["27608.34","0.000281"],)"
                                 R"(["27613.3","0.000277"],)"
                                 R"(["27614.1","0.027437"],)"
                                 R"(["27614.23","0.000389"],)"
                                 R"(["27616.22","0.056434"],)"
                                 R"(["27617.52","0.000541"],)"
                                 R"(["27620","0.001866"],)"
                                 R"(["27622.81","0.000999"],)"
                                 R"(["27623.58","0.002293"],)"
                                 R"(["27623.59","0.027438"],)"
                                 R"(["27624.94","0.038394"],)"
                                 R"(["27625","0.008834"],)"
                                 R"(["27625.6","0.000309"],)"
                                 R"(["27626.36","0.001443"],)"
                                 R"(["27626.4","0.000412"],)"
                                 R"(["27626.52","0.001011"],)"
                                 R"(["27626.8","0.001712"],)"
                                 R"(["27628.15","0.001814"],)"
                                 R"(["27628.71","0.00142"],)"
                                 R"(["27629.01","0.000543"],)"
                                 R"(["27630.96","0.010623"],)"
                                 R"(["27631.94","0.000771"],)"
                                 R"(["27633.48","0.000451"],)"
                                 R"(["27636.3","0.002636"],)"
                                 R"(["27639.66","0.007106"],)"
                                 R"(["27641.85","0.000381"],)"
                                 R"(["27643.24","0.000435"],)"
                                 R"(["27644.62","0.000193"],)"
                                 R"(["27644.99","0.000942"],)"
                                 R"(["27650","0.003"],)"
                                 R"(["27650.87","0.000849"],)"
                                 R"(["27654.96","0.000592"],)"
                                 R"(["27660.81","0.000136"],)"
                                 R"(["27663.15","0.004704"],)"
                                 R"(["27666.35","0.000554"],)"
                                 R"(["27666.52","0.000363"],)"
                                 R"(["27666.64","0.00034"],)"
                                 R"(["27670.21","0.001161"])"
                                 R"( ],)"
                                 R"("u":1323396,)"
                                 R"("seq":792472629)"
                                 R"(})"
                                 R"(})";
auto const MESSAGE_50_DELTA = R"({)"
                              R"("topic":"orderbook.50.BTCUSDT",)"
                              R"("ts":1682083231922,)"
                              R"("type":"delta",)"
                              R"("data":{)"
                              R"( "s":"BTCUSDT",)"
                              R"("b":[)"
                              R"(["27579.99","0.031244"])"
                              R"(],)"
                              R"("a":[],)"
                              R"("u":1323397,)"
                              R"("seq":792472763)"
                              R"(})"
                              R"(})";
}  // namespace

TEST_CASE("json_order_book_simple_1", "[json_order_book]") {
  core::Buffer buffer(8192);
  json::OrderBook order_book{MESSAGE_1, buffer};
}

TEST_CASE("json_order_book_simple_50_snapshot", "[json_order_book]") {
  core::Buffer buffer(8192);
  json::OrderBook order_book{MESSAGE_50_SNAPSHOT, buffer};
}

TEST_CASE("json_order_book_simple_50_delta", "[json_order_book]") {
  core::Buffer buffer(8192);
  json::OrderBook order_book{MESSAGE_50_DELTA, buffer};
}

TEST_CASE("json_order_book_parser", "[json_order_book]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { FAIL(); }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { found = true; }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
    void operator()(Trace<json::Tickers> const &) override { FAIL(); }
    // private
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Wallet> const &) override { FAIL(); }
    void operator()(Trace<json::Position> const &) override { FAIL(); }
    void operator()(Trace<json::Order> const &) override { FAIL(); }
    void operator()(Trace<json::Execution2> const &) override { FAIL(); }

    bool found = false;
  } handler;
  core::Buffer buffer(8192);
  core::json::Buffer buffer_2{buffer};
  auto res = json::Parser::dispatch(handler, MESSAGE_1, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}
