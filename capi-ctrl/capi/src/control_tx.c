/* ======================================================================== */
/**
   @file control_tx.cpp

   C source file to implement the Audio Post Processor Interface for
   DECIMATE example
 */

/* =========================================================================
   Copyright (c) 2015-2021 QUALCOMM Technologies Incorporated.
   All rights reserved. Qualcomm Technologies Proprietary and Confidential.
   ========================================================================= */
/*------------------------------------------------------------------------
 * Include files and Macro definitions
 * -----------------------------------------------------------------------*/

#ifndef _DEBUG
#define _DEBUG
#endif
#include "control_tx.h"
#include "control_tx_utils.h"
#include "assert.h"
#include <hexagon_protos.h>

//IMCL
 #include "capi_cmn_imcl_utils.h"
 #include "capi_intf_extn_imcl.h"
 #include "capi_cmn_ctrl_port_list.h"
 #include "capi_cmn.h"
 //#include "imcl_decimate_api.h"
 //#include "imcl_gain_api.h"



	
/*------------------------------------------------------------------------
 * Static declarations
 * -----------------------------------------------------------------------*/

static capi_err_t control_tx_process(capi_t *_pif, capi_stream_data_t *input[], capi_stream_data_t *output[]);

static capi_err_t control_tx_end(capi_t *_pif);

static capi_err_t control_tx_set_param(capi_t *                _pif,
                                          uint32_t                param_id,
                                          const capi_port_info_t *port_info_ptr,
                                          capi_buf_t *            params_ptr);

static capi_err_t control_tx_get_param(capi_t *                _pif,
                                          uint32_t                param_id,
                                          const capi_port_info_t *port_info_ptr,
                                          capi_buf_t *            params_ptr);

static capi_err_t control_tx_set_properties(capi_t *_pif, capi_proplist_t *props_ptr);

static capi_err_t control_tx_get_properties(capi_t *_pif, capi_proplist_t *props_ptr);

/** Virtual table for control_tx
 *  */
static capi_vtbl_t vtbl = { control_tx_process,        control_tx_end,
                            control_tx_set_param,      control_tx_get_param,
                            control_tx_set_properties, control_tx_get_properties };

#if 0                           
static capi_err_t decimate_send_data_over_imc(control_tx_t * me_ptr,  uint32 data_over_imc);                           
                            

static capi_err_t decimate_send_data_over_imc(control_tx_t * me_ptr,  uint32 data_over_imc)
{
	capi_err_t result = CAPI_EOK;
	capi_buf_t buf;
	uint32_t control_port_id = 0;
	imcl_port_state_t port_state = CTRL_PORT_CLOSE;
	ctrl_port_data_t *port_data_ptr = NULL;
	buf.actual_data_len = sizeof(decimate_imcl_header_t) + sizeof(capi_decimate_control_data_payload_t);
	buf.data_ptr = NULL;
	buf.max_data_len = 0;
	imcl_outgoing_data_flag_t flags;
	flags.should_send = TRUE;
	flags.is_trigger = TRUE;
 
	// Get the first control port id for the intentMODULE_INTENT_ID_FOR_IMCL
	capi_cmn_ctrl_port_list_get_next_port_data(&me_ptr->ctrl_port_info, INTENT_ID_DECIMATE_CONTROL,  control_port_id, &me_ptr->ctrl_port_ptr);
	if (port_data_ptr)
	{
		control_port_id = port_data_ptr->port_info.port_id;
		port_state = port_data_ptr->state;
	}
	else
	{
		AR_MSG(DBG_ERROR_PRIO,"Port data ptr doesnt exist");
	}
	if (CTRL_PORT_PEER_CONNECTED == port_state)
	{
		// Get one time buf from the queue
		result |= capi_cmn_imcl_get_one_time_buf(&me_ptr->cb_info, control_port_id, buf.actual_data_len, &buf);
		if (CAPI_FAILED(result) || NULL == buf.data_ptr)
		{
			AR_MSG(DBG_ERROR_PRIO,"Getting one time buffer failed");
			return result;
		}
		decimate_imcl_header_t *out_cfg_ptr = (decimate_imcl_header_t *)buf.data_ptr;
		capi_decimate_control_data_payload_t *data_over_imc_payload = (capi_decimate_control_data_payload_t*)(out_cfg_ptr + 1);
		out_cfg_ptr->opcode = PARAM_ID_DECIMATE_CONTROL_IMC_PAYLOAD;
		out_cfg_ptr->actual_data_len = sizeof(decimate_imcl_header_t);
		data_over_imc_payload->decimation_factor = data_over_imc;
		// send data to peer/destination module
		if (CAPI_SUCCEEDED(capi_cmn_imcl_send_to_peer(&me_ptr->cb_info,  &buf, control_port_id, flags)))
		{
			AR_MSG(DBG_HIGH_PRIO,"Data %d sent to control port 0x%x",data_over_imc_payload->decimation_factor, control_port_id);
		}
	}
	else
	{
		AR_MSG(DBG_ERROR_PRIO,"Control port is not connected");
	}
	return result;
}
#endif
/*------------------------------------------------------------------------
  Function name: control_tx_get_static_properties
  DESCRIPTION: Function to get the static properties of DECIMATE module
  -----------------------------------------------------------------------*/
capi_err_t control_tx_get_static_properties(capi_proplist_t *init_set_properties, capi_proplist_t *static_properties)
{
   capi_err_t capi_result = CAPI_EOK;

   if (NULL != static_properties)
   {
      capi_result = control_tx_process_get_properties((control_tx_t *)NULL, static_properties);
      if (CAPI_FAILED(capi_result))
      {
         return capi_result;
      }
   }

   if (NULL != init_set_properties)
   {
      AR_MSG(DBG_HIGH_PRIO, "CAPI DECIMATE CTRL: get static properties ignoring init_set_properties!");
   }

   return capi_result;
}

/*------------------------------------------------------------------------
  Function name: control_tx_init
  DESCRIPTION: Initialize the CAPI Decimate module and library.
  This function can allocate memory.
  -----------------------------------------------------------------------*/
capi_err_t control_tx_init(capi_t *_pif, capi_proplist_t *init_set_properties)
{
   capi_err_t capi_result = CAPI_EOK;

   if (NULL == _pif || NULL == init_set_properties)
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI DECIMATE CTRL: "
             "Init received bad pointer, 0x%lx, 0x%lx",
             (uint32_t)_pif,
             (uint32_t)init_set_properties);

      return CAPI_EBADPARAM;
   }

   int8_t *         ptr           = (int8_t *)_pif;
   control_tx_t *me_ptr        = (control_tx_t *)ptr;
   uint32_t         decimate_size = 0;

   // Each block must begin with 8-byte aligned memory.
   decimate_size = decimate_align_to_8_byte(sizeof(control_tx_t)) +
                   decimate_align_to_8_byte(TEMP_BUF_SIZE_IN_SAMPLES * sizeof(int16_t))    // For L-channel
                   + decimate_align_to_8_byte(TEMP_BUF_SIZE_IN_SAMPLES * sizeof(int16_t)); // For R-channel

   // Initialize the memory to 0
   memset((void *)me_ptr, 0, decimate_size);

   /* Shift the memory pointer by size of control_tx_t
      for other members of the structure.
    */
   ptr += decimate_align_to_8_byte(sizeof(control_tx_t));

   // Allocate memory to output samples after decimation
   me_ptr->out_samples = (uint32_t *)ptr;
   memset(me_ptr->out_samples, 0, sizeof(uint32_t));

   // Disable the module by default
   me_ptr->enable_flag                 = 1;
   me_ptr->lib_config.decimatest_frame = 1;

   control_tx_init_media_fmt(me_ptr);
   me_ptr->vtbl.vtbl_ptr = &vtbl;

   me_ptr->decimation_factor = 1;
   
   /* Initialize gain configuration to defaults*/
   uint16_t apply_gain          = 0x2000;
   me_ptr->gain_config.gain_q12 = apply_gain >> 1;
   me_ptr->gain_config.gain_q13 = apply_gain;


   //IMCL
   me_ptr->is_decimate_ctrl_port_received = DECIMATE_CTRL_PORT_INFO_NOT_RCVD;
   me_ptr->is_gain_ctrl_port_received = GAIN_CTRL_PORT_INFO_NOT_RCVD;
   //Initialize the control port list
   capi_cmn_ctrl_port_list_init(&me_ptr->ctrl_port_info);
	
   // should contain  EVENT_CALLBACK_INFO, PORT_INFO
   capi_result = control_tx_process_set_properties(me_ptr, init_set_properties);

   // Ignoring non-fatal error code.
   capi_result ^= (capi_result & CAPI_EUNSUPPORTED);
   if (CAPI_FAILED(capi_result))
   {
      return capi_result;
   }

   AR_MSG(DBG_HIGH_PRIO, "CAPI DECIMATE CTRL: Init done!");
   return capi_result;
}

/*------------------------------------------------------------------------
  Function name: control_tx_process
  DESCRIPTION: Processes an input buffer and generates an output buffer.
  -----------------------------------------------------------------------*/
static capi_err_t control_tx_process(capi_t *_pif, capi_stream_data_t *input[], capi_stream_data_t *output[])
{

   capi_err_t       capi_result = CAPI_EOK;

   memcpy(output[0]->buf_ptr[0].data_ptr, input[0]->buf_ptr[0].data_ptr, input[0]->buf_ptr->actual_data_len);
   if (2 == output[0]->bufs_num)
   {
      memcpy(output[0]->buf_ptr[1].data_ptr, input[0]->buf_ptr[1].data_ptr, input[0]->buf_ptr->actual_data_len);
   }
   // Update actual data length for output buffer
   output[0]->buf_ptr[0].actual_data_len = input[0]->buf_ptr->actual_data_len;
   if (2 == output[0]->bufs_num)
   {
      output[0]->buf_ptr[1].actual_data_len = input[0]->buf_ptr->actual_data_len;
   }

   // Copy flags from input to output
   output[0]->flags = input[0]->flags;

   return capi_result;
}

/*------------------------------------------------------------------------
  Function name: control_tx_end
  DESCRIPTION: Returns the library to the uninitialized state and frees the
  memory that was allocated by init(). This function also frees the virtual
  function table.
  -----------------------------------------------------------------------*/
static capi_err_t control_tx_end(capi_t *_pif)
{
   capi_err_t capi_result = CAPI_EOK;
   if (NULL == _pif)
   {
      AR_MSG(DBG_ERROR_PRIO, "CAPI DECIMATE CTRL: End received bad pointer, 0x%lx", (uint32_t)_pif);
      return CAPI_EBADPARAM;
   }

   control_tx_t *me_ptr = (control_tx_t *)(_pif);

   control_tx_release_memory(me_ptr);

   me_ptr->vtbl.vtbl_ptr = NULL;

   AR_MSG(DBG_HIGH_PRIO, "CAPI DECIMATE CTRL: End done");
   return capi_result;
}

/*------------------------------------------------------------------------
  Function name: control_tx_set_param
  DESCRIPTION: Sets either a parameter value or a parameter structure containing
  multiple parameters. In the event of a failure, the appropriate error code is
  returned.
  -----------------------------------------------------------------------*/
static capi_err_t control_tx_set_param(capi_t *                _pif,
                                          uint32_t                param_id,
                                          const capi_port_info_t *port_info_ptr,
                                          capi_buf_t *            params_ptr)
{
   if (NULL == _pif || NULL == params_ptr)
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI CONTROL: Set param received bad pointer, 0x%lx, 0x%lx",
             (uint32_t)_pif,
             (uint32_t)params_ptr);
      return CAPI_EBADPARAM;
   }

   capi_err_t       capi_result = CAPI_EOK;
   control_tx_t *me_ptr      = (control_tx_t *)(_pif);

   switch (param_id)
   {

      case PARAM_ID_MODULE_ENABLE:
      {
         if (params_ptr->actual_data_len >= sizeof(param_id_module_enable_t))
         {
            param_id_module_enable_t *cfg_ptr = (param_id_module_enable_t *)(params_ptr->data_ptr);
            // set DECIMATE enable flag
            me_ptr->enable_flag = cfg_ptr->enable;

            /* After enabling/disabling the module, use callback functionality
             * to update the caller service about the process state of module.
             */
            control_tx_raise_event(me_ptr);
            AR_MSG(DBG_HIGH_PRIO, "CAPI Decimate CTRL: <<set_param>> Enable/Disable %lu ", me_ptr->enable_flag);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO, "CAPI Decimate CTRL: <<set_param>> Bad param size %lu  ", params_ptr->actual_data_len);
            return CAPI_ENEEDMORE;
         }
         break;
      }

      case CAPI_PARAM_ID_UPDATE_DECIMATION_FACTOR:
      {
         if (params_ptr->actual_data_len >= sizeof(decimate_factor_cfg_t))
         {
             decimate_factor_cfg_t *cfg_ptr = (decimate_factor_cfg_t *)(params_ptr->data_ptr);
            if (1 <= cfg_ptr->decimation_factor)
            {
               if (cfg_ptr->decimation_factor != me_ptr->decimation_factor)
               {
                  me_ptr->decimation_factor = cfg_ptr->decimation_factor;
                  AR_MSG(DBG_HIGH_PRIO,
                         "CAPI DECIMATE CTRL CTRL: <<set_param>> Decimation factor set to %lu",
                         me_ptr->decimation_factor);
                  if (me_ptr->is_mf_received)
                  {
                     // we need to raise output media format because it depends on decimation factor as well, and we
                     // need to update output media format everytime we get a new deciamtion factor
                     //control_tx_raise_output_media_format_event(me_ptr);
                     control_tx_raise_event(me_ptr);
                  }
               }
               else
               {
                  AR_MSG(DBG_HIGH_PRIO,
                         "CAPI DECIMATE CTRL CTRL: Same decimation factor received.Decimation factor = %lu",
                         me_ptr->decimation_factor);
                  return CAPI_EOK;
               }
               
               ///////////// Calling IMCL stuff here
               
               capi_err_t result = CAPI_EOK;
         	capi_buf_t buf;
         	uint32_t control_port_id = 0;
         	imcl_port_state_t port_state = CTRL_PORT_CLOSE;
   
         	buf.actual_data_len = sizeof(decimate_imcl_header_t) + sizeof(capi_decimate_control_data_payload_t);
         	buf.data_ptr = NULL;
         	buf.max_data_len = 0;

         	imcl_outgoing_data_flag_t flags;
         	flags.should_send = TRUE;
         	flags.is_trigger = FALSE;
   
         	// Get the first control port id for the intent #INTENT_ID_DECIMATE_CONTROL
         	capi_cmn_ctrl_port_list_get_next_port_data(&me_ptr->ctrl_port_info,
                                              INTENT_ID_DECIMATE_CONTROL,
                                              control_port_id, // initially, an invalid port id
                                              &me_ptr->ctrl_port_ptr);

         	if (me_ptr->ctrl_port_ptr)
         	{
            		control_port_id = me_ptr->ctrl_port_ptr->port_info.port_id;
            		port_state = me_ptr->ctrl_port_ptr->state;
         	}
         	else
         	{
            		AR_MSG_ISLAND(DBG_ERROR_PRIO,"Port data ptr doesnt exist. ctrl port id=0x%x port state = 0x%x",control_port_id, port_state);
         	}

         	if (0 != control_port_id) {
            		me_ptr->is_decimate_ctrl_port_received = DECIMATE_CTRL_PORT_INFO_RCVD;
            		if (CTRL_PORT_PEER_CONNECTED == port_state)
            		{
               		// Get one time buf from the queue
               		result |= capi_cmn_imcl_get_one_time_buf(&me_ptr->cb_info, control_port_id, buf.actual_data_len, &buf);
      
               		AR_MSG_ISLAND(DBG_ERROR_PRIO,"buf.actual_data_len=0x%x", buf.actual_data_len);

               		if (CAPI_FAILED(result) || NULL == buf.data_ptr)
               		{
                  			AR_MSG(DBG_ERROR_PRIO,"Getting one time buffer failed");
                  			return result;
               		}
               		decimate_imcl_header_t *out_cfg_ptr = (decimate_imcl_header_t *)buf.data_ptr;
               		capi_decimate_control_data_payload_t *data_over_imc_payload = (capi_decimate_control_data_payload_t*)(out_cfg_ptr + 1);

               		out_cfg_ptr->opcode = PARAM_ID_DECIMATE_CONTROL_IMC_PAYLOAD;
               		out_cfg_ptr->actual_data_len = sizeof(capi_decimate_control_data_payload_t);
               		data_over_imc_payload->decimation_factor = me_ptr->decimation_factor;
               		// send data to peer/destination module
               		if (CAPI_SUCCEEDED(capi_cmn_imcl_send_to_peer(&me_ptr->cb_info, &buf, control_port_id, flags)))
               		{
                  			AR_MSG(DBG_HIGH_PRIO,"Enable %d and dec factor %d sent to control port 0x%x", data_over_imc_payload->decimation_factor, control_port_id);
               		}
            		}
            		else
            		{
               		AR_MSG(DBG_ERROR_PRIO,"Control port is not connected");
            		}              
         	}
         	else {
			AR_MSG(DBG_ERROR_PRIO,"Control port id is not proper");
	 	}
            	/////////////////////////// IMCL ends here            	
               
               
            }
            else
            {
               AR_MSG(DBG_ERROR_PRIO,
                      "CAPI DECIMATE CTRL: <<set_param>> "
                      "Decimation factor should be greater than 1 ");
               return CAPI_EUNSUPPORTED;
            }
            
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO, "CAPI DECIMATE CTRL: <<set_param>> Bad param size %lu", params_ptr->actual_data_len);
            return CAPI_ENEEDMORE;
         }
         break;
      }

      case CAPI_PARAM_ID_UPDATE_DECIMATION_COEFFS:
      {
         if (params_ptr->actual_data_len >= sizeof(control_tx_coeff_arr_t))
         {
		control_tx_coeff_arr_t *cfg_ptr = (control_tx_coeff_arr_t *)(params_ptr->data_ptr);
		memcpy(me_ptr->coeff_val, cfg_ptr->coeff_val, sizeof(cfg_ptr->coeff_val));             
             
		///////////// Calling IMCL stuff here
               
		capi_err_t result = CAPI_EOK;
		capi_buf_t buf;
		uint32_t control_port_id = 0;
		imcl_port_state_t port_state = CTRL_PORT_CLOSE;
   		buf.actual_data_len = sizeof(decimate_imcl_header_t) + sizeof(capi_decimate_coeff_arr_payload_t);
         	buf.data_ptr = NULL;
         	buf.max_data_len = 0;

         	imcl_outgoing_data_flag_t flags;
         	flags.should_send = TRUE;
         	flags.is_trigger = FALSE;
   
         	// Get the first control port id for the intent #INTENT_ID_DECIMATE_CONTROL
         	capi_cmn_ctrl_port_list_get_next_port_data(&me_ptr->ctrl_port_info,
                                              INTENT_ID_DECIMATE_CONTROL,
                                              control_port_id, // initially, an invalid port id
                                              &me_ptr->ctrl_port_ptr);

         	if (me_ptr->ctrl_port_ptr)
         	{
            		control_port_id = me_ptr->ctrl_port_ptr->port_info.port_id;
            		port_state = me_ptr->ctrl_port_ptr->state;
         	}
         	else
         	{
            		AR_MSG_ISLAND(DBG_ERROR_PRIO,"Port data ptr doesnt exist. ctrl port id=0x%x port state = 0x%x",control_port_id, port_state);
         	}

         	if (0 != control_port_id) {
            		me_ptr->is_decimate_ctrl_port_received = DECIMATE_CTRL_PORT_INFO_RCVD;
            		if (CTRL_PORT_PEER_CONNECTED == port_state)
            		{
               		// Get one time buf from the queue
               		result |= capi_cmn_imcl_get_one_time_buf(&me_ptr->cb_info, control_port_id, buf.actual_data_len, &buf);
      
               		AR_MSG_ISLAND(DBG_ERROR_PRIO,"buf.actual_data_len=0x%x", buf.actual_data_len);

               		if (CAPI_FAILED(result) || NULL == buf.data_ptr)
               		{
                  			AR_MSG(DBG_ERROR_PRIO,"Getting one time buffer failed");
                  			return result;
               		}
               		decimate_imcl_header_t *out_cfg_ptr = (decimate_imcl_header_t *)buf.data_ptr;
               		capi_decimate_coeff_arr_payload_t *data_over_imc_payload = (capi_decimate_coeff_arr_payload_t*)(out_cfg_ptr + 1);

               		out_cfg_ptr->opcode = PARAM_ID_DECIMATE_COEFF_IMC_PAYLOAD;
               		out_cfg_ptr->actual_data_len = sizeof(capi_decimate_coeff_arr_payload_t);
               		memcpy(data_over_imc_payload->coeff_val, me_ptr->coeff_val, sizeof(me_ptr->coeff_val));
               		// send data to peer/destination module
               		if (CAPI_SUCCEEDED(capi_cmn_imcl_send_to_peer(&me_ptr->cb_info, &buf, control_port_id, flags)))
               		{
                  			//AR_MSG(DBG_HIGH_PRIO,"Enable %d and dec factor %d sent to control port 0x%x", data_over_imc_payload->coeff_val[0], control_port_id);
               		}
            		}
            		else
            		{
               		AR_MSG(DBG_ERROR_PRIO,"Control port is not connected");
            		}              
         	}
         	else {
			AR_MSG(DBG_ERROR_PRIO,"Control port id is not proper");
	 	}
            	/////////////////////////// IMCL ends here            	           
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO, "CAPI DECIMATE CTRL: <<set_param>> Bad param size %lu", params_ptr->actual_data_len);
            return CAPI_ENEEDMORE;
         }
         break;
      }

      case CAPI_PARAM_ID_UPDATE_MODULE_GAIN:
      {
         if (params_ptr->actual_data_len >= sizeof(param_id_module_gain_cfg_t))
         {
            param_id_module_gain_cfg_t *gain_cfg_ptr = (param_id_module_gain_cfg_t *)(params_ptr->data_ptr);
            me_ptr->gain_config.gain_q13             = gain_cfg_ptr->gain;
            me_ptr->gain_config.gain_q12             = gain_cfg_ptr->gain >> 1;

            /* Determine if gain module should be enabled
             * Module is enabled if PARAM_ID_MODULE_ENABLE was set to enable AND
             * if the gain value is not unity in Q13 format */
#if 0
            uint32_t enable = TRUE;

            if (Q13_UNITY_GAIN == me_ptr->gain_config.gain_q12 << 1)
            {
               enable = FALSE;
            }
            enable = enable && ((me_ptr->gain_config.enable == 0) ? 0 : 1);
#endif            
            
            
            ///////////// Calling IMCL stuff here
               
                capi_err_t result = CAPI_EOK;
         	capi_buf_t buf;
         	uint32_t control_port_id = 0;
         	imcl_port_state_t port_state = CTRL_PORT_CLOSE;
   
         	buf.actual_data_len = sizeof(gain_imcl_header_t) + sizeof(capi_gain_control_data_payload_t);
         	buf.data_ptr = NULL;
         	buf.max_data_len = 0;

         	imcl_outgoing_data_flag_t flags;
         	flags.should_send = TRUE;
         	flags.is_trigger = FALSE;
   
         	// Get the first control port id for the intent #INTENT_ID_GAIN_CONTROL
         	capi_cmn_ctrl_port_list_get_next_port_data(&me_ptr->ctrl_port_info,
                                              INTENT_ID_GAIN_CONTROL,
                                              control_port_id, // initially, an invalid port id
                                              &me_ptr->ctrl_port_ptr);

         	if (me_ptr->ctrl_port_ptr)
         	{
            		control_port_id = me_ptr->ctrl_port_ptr->port_info.port_id;
            		port_state = me_ptr->ctrl_port_ptr->state;
            		AR_MSG_ISLAND(DBG_HIGH_PRIO,"port id=0x%x port state = 0x%x",control_port_id, port_state);
         	}
         	else
         	{
            		AR_MSG_ISLAND(DBG_ERROR_PRIO,"Port data ptr doesnt exist. ctrl port id=0x%x port state = 0x%x",control_port_id, port_state);
         	}

         	if (0 != control_port_id) {
            		me_ptr->is_gain_ctrl_port_received = GAIN_CTRL_PORT_INFO_RCVD;
            		if (CTRL_PORT_PEER_CONNECTED == port_state)
            		{
               		// Get one time buf from the queue
               		result |= capi_cmn_imcl_get_one_time_buf(&me_ptr->cb_info, control_port_id, buf.actual_data_len, &buf);
      
               		AR_MSG_ISLAND(DBG_ERROR_PRIO,"buf.actual_data_len=0x%x", buf.actual_data_len);

               		if (CAPI_FAILED(result) || NULL == buf.data_ptr)
               		{
                  			AR_MSG(DBG_ERROR_PRIO,"Getting one time buffer failed");
                  			return result;
               		}
               		gain_imcl_header_t *out_cfg_ptr = (gain_imcl_header_t *)buf.data_ptr;
               		capi_gain_control_data_payload_t *data_over_imc_payload = (capi_gain_control_data_payload_t*)(out_cfg_ptr + 1);

               		out_cfg_ptr->opcode = PARAM_ID_GAIN_CONTROL_IMC_PAYLOAD;
               		out_cfg_ptr->actual_data_len = sizeof(capi_gain_control_data_payload_t);
               		data_over_imc_payload->gain = me_ptr->gain_config.gain_q13;
               		// send data to peer/destination module
               		if (CAPI_SUCCEEDED(capi_cmn_imcl_send_to_peer(&me_ptr->cb_info, &buf, control_port_id, flags)))
               		{
                  			AR_MSG(DBG_HIGH_PRIO,"Enable %d and dec factor %d sent to control port 0x%x", data_over_imc_payload->gain, control_port_id);
               		}
            		}
            		else
            		{
               		AR_MSG(DBG_ERROR_PRIO,"Control port is not connected");
            		}              
         	}
         	else {
			AR_MSG(DBG_ERROR_PRIO,"Control port id is not proper");
	 	}
            	/////////////////////////// IMCL ends here    
            
            
         }
         else
         {
            AR_MSG(DBG_HIGH_PRIO, "CAPI GAIN CTRL: PARAM_ID_GAIN %d", me_ptr->gain_config.gain_q13);
            CAPI_SET_ERROR(capi_result, CAPI_ENEEDMORE);
         }
         break;
      }
      
      case INTF_EXTN_PARAM_ID_IMCL_PORT_OPERATION:
      {
         uint32_t supported_intent[2] = {INTENT_ID_DECIMATE_CONTROL, INTENT_ID_GAIN_CONTROL};
         capi_result = capi_cmn_ctrl_port_operation_handler(
            &me_ptr->ctrl_port_info, params_ptr, 
            (POSAL_HEAP_ID)me_ptr->heap_info.heap_id, 0, 2, supported_intent);
         break;
      }
      default:
         AR_MSG(DBG_ERROR_PRIO, "CAPI CTRL: Set unsupported param ID 0x%x", (int)param_id);
         CAPI_SET_ERROR(capi_result, CAPI_EBADPARAM);
   }

   AR_MSG(DBG_HIGH_PRIO, "CAPI CTRL: Set param done");
   return capi_result;
}

/*------------------------------------------------------------------------
  Function name: control_tx_get_param
  DESCRIPTION: Gets either a parameter value or a parameter structure
  containing multiple parameters. In the event of a failure, the appropriate
  error code is returned.
 * -----------------------------------------------------------------------*/
static capi_err_t control_tx_get_param(capi_t *                _pif,
                                          uint32_t                param_id,
                                          const capi_port_info_t *port_info_ptr,
                                          capi_buf_t *            params_ptr)
{
   if (NULL == _pif || NULL == params_ptr)
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI DECIMATE CTRL: Get param received bad pointer, 0x%lx, 0x%lx",
             (uint32_t)_pif,
             (uint32_t)params_ptr);
      return CAPI_EBADPARAM;
   }

   capi_err_t       capi_result = CAPI_EOK;
   control_tx_t *me_ptr      = (control_tx_t *)(_pif);
   // void *param_payload_ptr  = (void *)(params_ptr->data_ptr);

   switch (param_id)
   {

      case PARAM_ID_MODULE_ENABLE:
      {
         if (params_ptr->max_data_len >= sizeof(param_id_module_enable_t))
         {
            param_id_module_enable_t *cfg_ptr = (param_id_module_enable_t *)(params_ptr->data_ptr);
            cfg_ptr->enable                   = me_ptr->enable_flag;
            params_ptr->actual_data_len       = sizeof(param_id_module_enable_t);
            AR_MSG(DBG_HIGH_PRIO,
                   "CAPI DECIMATE CTRL: <<get_param>> Enable/Disable %lu                                    ",
                   cfg_ptr->enable);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI DECIMATE CTRL: <<get_param>> Bad param size %lu                                    ",
                   params_ptr->max_data_len);
            return CAPI_ENEEDMORE;
         }
         break;
      }

      case CAPI_PARAM_ID_UPDATE_DECIMATION_FACTOR:
      {
         if (params_ptr->max_data_len >= sizeof(decimate_factor_cfg_t))
         {
            decimate_factor_cfg_t *cfg_ptr = (decimate_factor_cfg_t *)(params_ptr->data_ptr);
            cfg_ptr->decimation_factor     = me_ptr->decimation_factor;
            params_ptr->actual_data_len    = sizeof(decimate_factor_cfg_t);
            AR_MSG(DBG_HIGH_PRIO,
                   "CAPI DECIMATE CTRL: <<get_param>> Decimation factor = %lu                                              ",
                   cfg_ptr->decimation_factor);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI DECIMATE CTRL: <<get_param>> Bad param size %lu  Param id = %lu",
                   params_ptr->max_data_len,
                   param_id);
            return CAPI_ENEEDMORE;
         }
         break;
      }

      case CAPI_PARAM_ID_UPDATE_DECIMATION_COEFFS:
      {
         if (params_ptr->max_data_len >= sizeof(control_tx_coeff_arr_t))
         {
            control_tx_coeff_arr_t *cfg_ptr = (control_tx_coeff_arr_t *)(params_ptr->data_ptr);
            memcpy(cfg_ptr->coeff_val, me_ptr->coeff_val, sizeof(me_ptr->coeff_val));
            params_ptr->actual_data_len    = sizeof(control_tx_coeff_arr_t);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI DECIMATE CTRL: <<get_param>> Bad param size %lu  Param id = %lu",
                   params_ptr->max_data_len,
                   param_id);
            return CAPI_ENEEDMORE;
         }
         break;
      }

      case CAPI_PARAM_ID_UPDATE_MODULE_GAIN:
      {
         if (params_ptr->max_data_len >= sizeof(param_id_module_gain_cfg_t))
         {
            param_id_module_gain_cfg_t *gain_cfg_ptr = (param_id_module_gain_cfg_t *)(params_ptr->data_ptr);

            /* Fetch the gain values which were set to the lib*/
            gain_cfg_ptr->gain     = me_ptr->gain_config.gain_q13;
            gain_cfg_ptr->reserved = 0;
            AR_MSG(DBG_HIGH_PRIO, "CAPI GAIN CTRL:GET PARAM: gain = 0x%lx", gain_cfg_ptr->gain);
            /* Populate actual data length*/
            params_ptr->actual_data_len = sizeof(param_id_module_gain_cfg_t);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO, "CAPI GAIN CTRL: Get, Bad param size %lu", params_ptr->max_data_len);
            capi_result = CAPI_ENEEDMORE;
         }
         break;
      }
      
      default:
      {
         AR_MSG(DBG_ERROR_PRIO, "CAPI GAIN CTRL: Get unsupported param ID 0x%x", (int)param_id);
         CAPI_SET_ERROR(capi_result, CAPI_EBADPARAM);
      }
   }

   AR_MSG(DBG_HIGH_PRIO, "CAPI GAIN CTRL: Get param done");
   return capi_result;
}

/*------------------------------------------------------------------------
  Function name: control_tx_set_properties
  DESCRIPTION: Function to set the properties for the Decimate module
 * -----------------------------------------------------------------------*/
static capi_err_t control_tx_set_properties(capi_t *_pif, capi_proplist_t *props_ptr)
{
   capi_err_t capi_result = CAPI_EOK;
   if (NULL == _pif)
   {
      AR_MSG(DBG_ERROR_PRIO, "CAPI DECIMATE CTRL: Set properties received bad pointer, 0x%lx", (uint32_t)_pif);
      return CAPI_EBADPARAM;
   }
   control_tx_t *me_ptr = (control_tx_t *)_pif;
   capi_result             = control_tx_process_set_properties(me_ptr, props_ptr);
   return capi_result;
}

/*------------------------------------------------------------------------
  Function name: control_tx_get_properties
  DESCRIPTION: Function to get the properties for the Decimate module
 * -----------------------------------------------------------------------*/
static capi_err_t control_tx_get_properties(capi_t *_pif, capi_proplist_t *props_ptr)
{
   capi_err_t capi_result = CAPI_EOK;
   if (NULL == _pif)
   {
      AR_MSG(DBG_ERROR_PRIO, "CAPI DECIMATE CTRL: Get properties received bad pointer, 0x%lx", (uint32_t)_pif);
      return CAPI_EBADPARAM;
   }
   control_tx_t *me_ptr = (control_tx_t *)_pif;
   capi_result             = control_tx_process_get_properties(me_ptr, props_ptr);
   return capi_result;
}
