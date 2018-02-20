/*ckwg +29
 * Copyright 2018 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "downsample_process.h"

#include <kwiver_type_traits.h>

#include <vital/types/timestamp.h>

namespace kwiver
{

using sprokit::process;

create_config_trait( target_frame_rate, double, "1.0", "Target frame rate" );

class downsample_process::priv
{
public:
  explicit priv( downsample_process* p );
  ~priv();

  downsample_process* parent;

  double target_frame_rate;

  static port_t const port_inputs[5];
  static port_t const port_outputs[5];
};


process::port_t const downsample_process::priv::port_inputs[5] = {
  process::port_t( "input_1" ),
  process::port_t( "input_2" ),
  process::port_t( "input_3" ),
  process::port_t( "input_4" ),
  process::port_t( "input_5" ),
};

process::port_t const downsample_process::priv::port_outputs[5] = {
  process::port_t( "output_1" ),
  process::port_t( "output_2" ),
  process::port_t( "output_3" ),
  process::port_t( "output_4" ),
  process::port_t( "output_5" ),
};


downsample_process
::downsample_process( vital::config_block_sptr const& config )
  : process( config ),
    d( new downsample_process::priv( this ) )
{
  attach_logger( vital::get_logger( name() ) );

  make_ports();
  make_config();
}


downsample_process
::~downsample_process()
{
}


void downsample_process
::_configure()
{
  d->target_frame_rate = config_value_using_trait( target_frame_rate );
}


void downsample_process
::_init()
{
}


void downsample_process
::_step()
{
  kwiver::vital::timestamp ts;

  ts = grab_from_port_using_trait( timestamp );
  push_to_port_using_trait( timestamp, ts );

  for( size_t i = 0; i < 5; i++ )
  {
    if( has_input_port_edge( d->port_inputs[i] ) )
    {
      sprokit::edge_datum_t datum = grab_from_port( d->port_inputs[i] );
      push_to_port( d->port_outputs[i], datum );
    }
  }
}


void downsample_process
::make_ports()
{
  sprokit::process::port_flags_t optional;
  sprokit::process::port_flags_t required;

  required.insert( flag_required );

  declare_input_port_using_trait( timestamp, required );
  for( size_t i = 0; i < 5; i++ )
  {
    declare_input_port( priv::port_inputs[i],
                        type_any,
                        optional,
                        port_description_t( "Input data." ) );
  }

  declare_output_port_using_trait( timestamp, required );
  for( size_t i = 0; i < 5; i++ )
  {
    declare_output_port( priv::port_outputs[i],
                         type_any,
                         optional,
                         port_description_t( "Output data." ) );
  }
}


void downsample_process
::make_config()
{
  declare_config_using_trait( target_frame_rate );
}


downsample_process::priv
::priv( downsample_process* p )
  : parent( p )
{
}


downsample_process::priv
::~priv()
{
}

}
