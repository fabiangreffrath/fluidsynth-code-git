/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#include "fluid_chan.h"
#include "fluid_mod.h"
#include "fluid_synth.h"
#include "fluid_sfont.h"

#define SETCC(_c,_n,_v)  _c->cc[_n] = _v

static void fluid_channel_init(fluid_channel_t* chan);


/*
 * new_fluid_channel
 */
fluid_channel_t*
new_fluid_channel(fluid_synth_t* synth, int num)
{
  fluid_channel_t* chan;

  chan = FLUID_NEW(fluid_channel_t);
  if (chan == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    return NULL;
  }

  chan->synth = synth;
  chan->channum = num;
  chan->preset = NULL;

  fluid_channel_init(chan);
  fluid_channel_init_ctrl(chan, 0);

  return chan;
}

static void
fluid_channel_init(fluid_channel_t* chan)
{
  fluid_preset_t *newpreset;
  int prognum, banknum;

  prognum = 0;
  banknum = (chan->channum == 9)? 128 : 0; /* ?? */

  chan->prognum = prognum;
  chan->banknum = banknum;
  chan->sfontnum = 0;

  newpreset = fluid_synth_find_preset(chan->synth, banknum, prognum);
  fluid_channel_set_preset(chan, newpreset);

  chan->interp_method = FLUID_INTERP_DEFAULT;
  chan->tuning = NULL;
  chan->nrpn_select = 0;
  chan->nrpn_active = 0;
}

/*
  @param is_all_ctrl_off if nonzero, only resets some controllers, according to 
  http://www.midi.org/techspecs/rp15.php 
*/
void
fluid_channel_init_ctrl(fluid_channel_t* chan, int is_all_ctrl_off)
{
  int i;

  chan->key_pressure = 0;
  chan->channel_pressure = 0;
  chan->pitch_bend = 0x2000; /* Range is 0x4000, pitch bend wheel starts in centered position */

  for (i = 0; i < GEN_LAST; i++) {
    chan->gen[i] = 0.0f;
    chan->gen_abs[i] = 0;
  }

  if (is_all_ctrl_off) {
    for (i = 0; i < ALL_SOUND_OFF; i++) {
      if (i >= EFFECTS_DEPTH1 && i <= EFFECTS_DEPTH5) {
        continue;
      }
      if (i >= SOUND_CTRL1 && i <= SOUND_CTRL10) {
        continue;
      }
      if (i == BANK_SELECT_MSB || i == BANK_SELECT_LSB || i == VOLUME_MSB || 
          i == VOLUME_LSB || i == PAN_MSB || i == PAN_LSB) {
        continue;
      }

      SETCC(chan, i, 0);
    }
  }
  else {
    for (i = 0; i < 128; i++) {
      SETCC(chan, i, 0);
    }
  }

  /* Set RPN controllers to NULL state */
  SETCC(chan, RPN_LSB, 127);
  SETCC(chan, RPN_MSB, 127);

  /* Set NRPN controllers to NULL state */
  SETCC(chan, NRPN_LSB, 127);
  SETCC(chan, NRPN_MSB, 127);

  /* Expression (MSB & LSB) */
  SETCC(chan, EXPRESSION_MSB, 127);
  SETCC(chan, EXPRESSION_LSB, 127);

  if (!is_all_ctrl_off) {

    chan->pitch_wheel_sensitivity = 2; /* two semi-tones */

    /* Just like panning, a value of 64 indicates no change for sound ctrls */
    for (i = SOUND_CTRL1; i <= SOUND_CTRL10; i++) {
      SETCC(chan, i, 64);
    }

    /* Volume / initial attenuation (MSB & LSB) */
    SETCC(chan, VOLUME_MSB, 100);
    SETCC(chan, VOLUME_LSB, 0);

    /* Pan (MSB & LSB) */
    SETCC(chan, PAN_MSB, 64);
    SETCC(chan, PAN_LSB, 0);

    /* Reverb */
    /* SETCC(chan, EFFECTS_DEPTH1, 40); */
    /* Note: although XG standard specifies the default amount of reverb to 
       be 40, most people preferred having it at zero.
       See http://lists.gnu.org/archive/html/fluid-dev/2009-07/msg00016.html */
  }
}

/*
 * delete_fluid_channel
 */
int
delete_fluid_channel(fluid_channel_t* chan)
{
  if (chan->preset) delete_fluid_preset (chan->preset);
  FLUID_FREE(chan);
  return FLUID_OK;
}

void
fluid_channel_reset(fluid_channel_t* chan)
{
  fluid_channel_init(chan);
  fluid_channel_init_ctrl(chan, 0);
}

/*
 * fluid_channel_set_preset
 */
int
fluid_channel_set_preset(fluid_channel_t* chan, fluid_preset_t* preset)
{
  fluid_preset_notify(chan->preset, FLUID_PRESET_UNSELECTED, chan->channum);
  fluid_preset_notify(preset, FLUID_PRESET_SELECTED, chan->channum);

  if (chan->preset) delete_fluid_preset (chan->preset);
  chan->preset = preset;

  return FLUID_OK;
}

/*
 * fluid_channel_get_preset
 */
fluid_preset_t*
fluid_channel_get_preset(fluid_channel_t* chan)
{
  return chan->preset;
}

void
fluid_channel_set_sfontnum(fluid_channel_t* chan, int sfontnum)
{
  fluid_atomic_int_set (&chan->sfontnum, sfontnum);
}

int
fluid_channel_get_sfontnum(fluid_channel_t* chan)
{
  return fluid_atomic_int_get (&chan->sfontnum);
}

/*
 * fluid_channel_set_banknum
 */
void
fluid_channel_set_banknum(fluid_channel_t* chan, int banknum)
{
  fluid_atomic_int_set (&chan->banknum, banknum);
}

/*
 * fluid_channel_get_banknum
 */
int
fluid_channel_get_banknum(fluid_channel_t* chan)
{
  return fluid_atomic_int_get (&chan->banknum);
}

/*
 * fluid_channel_set_prognum
 */
void
fluid_channel_set_prognum(fluid_channel_t* chan, int prognum)
{
  fluid_atomic_int_set (&chan->prognum, prognum);
}

/*
 * fluid_channel_get_prognum
 */
int
fluid_channel_get_prognum(fluid_channel_t* chan)
{
  return fluid_atomic_int_get (&chan->prognum);
}

/* Set MIDI custom controller value for a channel */
void
fluid_channel_set_cc(fluid_channel_t* chan, int num, int val)
{
  if (num >= 0 && num < 128)
    fluid_atomic_int_set (&chan->cc[num], val);
}

/* Get MIDI custom controller value for a channel */
int
fluid_channel_get_cc(fluid_channel_t* chan, int num)
{
  return ((num >= 0) && (num < 128)) ? fluid_atomic_int_get (&chan->cc[num]) : 0;
}

/*
 * fluid_channel_get_num
 */
int
fluid_channel_get_num(fluid_channel_t* chan)
{
  return chan->channum;
}

/*
 * Sets the index of the interpolation method used on this channel,
 * as in fluid_interp in fluidsynth.h
 */
void fluid_channel_set_interp_method(fluid_channel_t* chan, int new_method)
{
  fluid_atomic_int_set (&chan->interp_method, new_method);
}

/*
 * Returns the index of the interpolation method used on this channel,
 * as in fluid_interp in fluidsynth.h
 */
int fluid_channel_get_interp_method(fluid_channel_t* chan)
{
  return fluid_atomic_int_get (&chan->interp_method);
}
