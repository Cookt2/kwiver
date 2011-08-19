/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "process.h"
#include "process_exception.h"

#include "config.h"
#include "datum.h"
#include "edge.h"
#include "stamp.h"
#include "types.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>

#include <map>
#include <utility>

/**
 * \file process.cxx
 *
 * \brief Implementation of the base class for \link vistk::process processes\endlink.
 */

namespace vistk
{

process::port_t const process::port_heartbeat = port_t("heartbeat");
config::key_t const process::config_name = config::key_t("_name");
config::key_t const process::config_type = config::key_t("_type");
process::port_type_t const process::type_any = port_type_t("_any");
process::port_type_t const process::type_none = port_type_t("_none");
process::port_flag_t const process::flag_output_const = port_flag_t("_const");
process::port_flag_t const process::flag_input_mutable = port_flag_t("_mutable");
process::port_flag_t const process::flag_required = port_flag_t("_required");

process::port_info
::port_info(port_type_t const& type_,
            port_flags_t const& flags_,
            port_description_t const& description_)
  : type(type_)
  , flags(flags_)
  , description(description_)
{
}

process::port_info
::~port_info()
{
}

process::conf_info
::conf_info(config::value_t const& def_,
            config::description_t const& description_)
  : def(def_)
  , description(description_)
{
}

process::conf_info
::~conf_info()
{
}

process::data_info
::data_info(bool same_color_,
            bool in_sync_,
            datum::datum_type_t max_status_)
  : same_color(same_color_)
  , in_sync(in_sync_)
  , max_status(max_status_)
{
}

process::data_info
::~data_info()
{
}

class process::priv
{
  public:
    priv();
    ~priv();

    void run_heartbeat();

    name_t name;
    process_registry::type_t type;

    conf_info_t name_conf_info;
    conf_info_t type_conf_info;

    typedef std::pair<edge_t, edge_t> edge_pair_t;
    typedef std::map<port_t, edge_pair_t> edge_map_t;

    edge_group_t heartbeats;

    port_info_t heartbeat_port_info;

    bool is_complete;

    stamp_t hb_stamp;

    static config::value_t const DEFAULT_PROCESS_NAME;
};

config::key_t const process::priv::DEFAULT_PROCESS_NAME = "(unnamed)";

void
process
::init()
{
  _init();
}

void
process
::step()
{
  /// \todo Make reentrant.

  /// \todo Are there any pre-_step actions?

  if (d->is_complete)
  {
    /// \todo What exactly should be done here?
  }
  else
  {
    _step();
  }

  /// \todo Are there any post-_step actions?

  d->run_heartbeat();
}

bool
process
::is_reentrant() const
{
  return false;
}

void
process
::connect_input_port(port_t const& port, edge_t edge)
{
  if (!edge)
  {
    throw null_edge_port_connection_exception(d->name, port);
  }

  _connect_input_port(port, edge_ref_t(edge));
}

void
process
::connect_output_port(port_t const& port, edge_t edge)
{
  if (!edge)
  {
    throw null_edge_port_connection_exception(d->name, port);
  }

  if (port == port_heartbeat)
  {
    d->heartbeats.push_back(edge_ref_t(edge));

    return;
  }

  _connect_output_port(port, edge_ref_t(edge));
}

process::ports_t
process
::input_ports() const
{
  ports_t ports = _input_ports();

  return ports;
}

process::ports_t
process
::output_ports() const
{
  ports_t ports = _output_ports();

  ports.push_back(port_heartbeat);

  return ports;
}

process::port_info_t
process
::input_port_info(port_t const& port) const
{
  return _input_port_info(port);
}

process::port_info_t
process
::output_port_info(port_t const& port) const
{
  if (port == port_heartbeat)
  {
    return d->heartbeat_port_info;
  }

  return _output_port_info(port);
}

config::keys_t
process
::available_config() const
{
  config::keys_t keys = _available_config();

  keys.push_back(config_name);
  keys.push_back(config_type);

  return keys;
}

process::conf_info_t
process
::config_info(config::key_t const& key) const
{
  if (key == config_name)
  {
    return d->name_conf_info;
  }
  if (key == config_type)
  {
    return d->type_conf_info;
  }

  return _config_info(key);
}

process::name_t
process
::name() const
{
  return d->name;
}

process_registry::type_t
process
::type() const
{
  return d->type;
}

process
::process(config_t const& config)
{
  if (!config)
  {
    throw null_process_config_exception();
  }

  d = boost::shared_ptr<priv>(new priv);

  d->name = config->get_value<name_t>(config_name, priv::DEFAULT_PROCESS_NAME);
  d->type = config->get_value<process_registry::type_t>(config_type);
}

process
::~process()
{
}

void
process
::_init()
{
}

void
process
::_step()
{
}

void
process
::_connect_input_port(port_t const& port, edge_ref_t /*edge*/)
{
  throw no_such_port_exception(d->name, port);
}

void
process
::_connect_output_port(port_t const& port, edge_ref_t /*edge*/)
{
  throw no_such_port_exception(d->name, port);
}

process::ports_t
process
::_input_ports() const
{
  return ports_t();
}

process::ports_t
process
::_output_ports() const
{
  return ports_t();
}

process::port_info_t
process
::_input_port_info(port_t const& port) const
{
  throw no_such_port_exception(d->name, port);
}

process::port_info_t
process
::_output_port_info(port_t const& port) const
{
  throw no_such_port_exception(d->name, port);
}

config::keys_t
process
::_available_config() const
{
  return config::keys_t();
}

process::conf_info_t
process
::_config_info(config::key_t const& key) const
{
  throw unknown_configuration_value_exception(d->name, key);
}

void
process
::mark_as_complete()
{
  d->is_complete = true;
}

stamp_t
process
::heartbeat_stamp() const
{
  return d->hb_stamp;
}

bool
process
::same_colored_data(edge_data_t const& data)
{
  return edge_data_info(data)->same_color;
}

bool
process
::syncd_data(edge_data_t const& data)
{
  return edge_data_info(data)->in_sync;
}

datum::datum_type_t
process
::max_status(edge_data_t const& data)
{
  return edge_data_info(data)->max_status;
}

process::data_info_t
process
::edge_data_info(edge_data_t const& data)
{
  bool same_color = true;
  bool in_sync = true;
  datum::datum_type_t max_type = datum::DATUM_INVALID;

  edge_datum_t const& fst = data[0];
  stamp_t const& st = fst.get<1>();

  BOOST_FOREACH (edge_datum_t const& dat, data)
  {
    datum::datum_type_t const type = dat.get<0>()->type();

    if (max_type < type)
    {
      max_type = type;
    }

    stamp_t const& st2 = dat.get<1>();

    if (!st->is_same_color(st2))
    {
      same_color = false;
    }
    if (*st != *st2)
    {
      in_sync = false;
    }
  }

  return data_info_t(new data_info(same_color, in_sync, max_type));
}

void
process
::push_to_edges(edge_group_t const& edges, edge_datum_t const& dat)
{
  BOOST_FOREACH (edge_ref_t const& e, edges)
  {
    edge_t const cur_edge = e.lock();

    cur_edge->push_datum(dat);
  }
}

edge_datum_t
process
::grab_from_edge_ref(edge_ref_t const& edge)
{
  edge_t const cur_edge = edge.lock();

  return cur_edge->get_datum();
}

process::priv
::priv()
  : is_complete(false)
  , hb_stamp(stamp::new_stamp())
{
  heartbeat_port_info = port_info_t(new port_info(
    type_none,
    port_flags_t(),
    port_description_t("Outputs the heartbeat stamp with an empty datum")));

  name_conf_info = conf_info_t(new conf_info(
    boost::lexical_cast<config::value_t>(priv::DEFAULT_PROCESS_NAME),
    config::description_t("The name of the process")));
  type_conf_info = conf_info_t(new conf_info(
    config::value_t(),
    config::description_t("The type of the process")));
}

process::priv
::~priv()
{
}

void
process::priv
::run_heartbeat()
{
  datum_t dat;

  if (is_complete)
  {
    dat = datum::complete_datum();
  }
  else
  {
    dat = datum::empty_datum();
  }

  edge_datum_t const edge_dat(dat, hb_stamp);

  process::push_to_edges(heartbeats, edge_dat);

  hb_stamp = stamp::incremented_stamp(hb_stamp);
}

}
