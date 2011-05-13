/*
 * Copyright 2010 HUAWEI Tech. Co., Ltd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * IRDA remote control driver
 *
 */
 #ifndef _IRDA_CTRL_NEC_PROTOCOL_H_
#define _IRDA_CTRL_NEC_PROTOCOL_H_

#define  REMOTER_USER_CODE  0x00ff0000 
         
#define  UP_ADJUST          400 
#define  DOWN_ADJUST        400      
#define  MIN_INTEVAL        500  

#define  REMOTER_START_CODE_TIME	13500   
#define  REMOTER_START_CODE_TIME_MAX	(REMOTER_START_CODE_TIME + 4/*3*/*UP_ADJUST )    
#define  REMOTER_START_CODE_TIME_MIN	(REMOTER_START_CODE_TIME - 6/*2*/*DOWN_ADJUST )    

#define  REMOTER_REPEAT_CODE_TIME	11810  
#define  REMOTER_REPEAT_CODE_TIME_MAX	(REMOTER_REPEAT_CODE_TIME + 3*UP_ADJUST)  
#define  REMOTER_REPEAT_CODE_TIME_MIN	(REMOTER_REPEAT_CODE_TIME - 3*DOWN_ADJUST )    

#define  REMOTER_LOW_CODE_TIME		1120   
#define  REMOTER_LOW_CODE_TIME_MAX	(REMOTER_LOW_CODE_TIME + UP_ADJUST )  
#define  REMOTER_LOW_CODE_TIME_MIN	(REMOTER_LOW_CODE_TIME - DOWN_ADJUST )   

#define  REMOTER_HIGH_CODE_TIME		2240   
#define  REMOTER_HIGH_CODE_TIME_MAX	(REMOTER_HIGH_CODE_TIME + UP_ADJUST )    
#define  REMOTER_HIGH_CODE_TIME_MIN	(REMOTER_HIGH_CODE_TIME - DOWN_ADJUST )    

#define  CATCH_TIMEOUT			100*1000   
#define  REPEAT_TIMEOUT			150*1000   
#define  CATCH_REPEAT_TIMEOUT  15*1000  

#define  IRDA_DATA_BITS       32   

#define  REPEAT_LOADER_CODE_TIMES  5  


#define  REMOTER_IDLE          0  
#define  REMOTER_CATCH_START   1   
#define  REMOTER_CATCH_DATA    2   
#define  REMOTER_CATCH_DATA_DONE    3   
#define  REMOTER_CATCH_REPEAT_START 4  
#define  REMOTER_CATCH_REPEAT_END   5   

#endif




