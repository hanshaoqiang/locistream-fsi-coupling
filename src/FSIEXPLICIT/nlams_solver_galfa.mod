	  <  $   k820309              11.0        �fDL                                                                                                           
       nlams_solver_generalalfa.f90 NLAMS_SOLVER_GALFA          @                                                  
                &                   &                                                      @                                    
                  @                                    
                  @                                    
                  @                                    
                  @                                    
                  @                                    
                  @                                    
                  @                               	     
                  @                               
     
                  @                                    
                  @                                    
                  @                                    
                  @                                    
       #         @                                                    #NLAMS_SHELLSOLVER_STEP_GALFA%ANSSIZE    #NLAMS_SHELLSOLVER_STEP_GALFA%MATMUL                 @                                                                                          MATMUL #         @                                                     #         @                                                     #         @                                                   #NLAMS_DELTA_FORCE%ANSSIZE    #NLAMS_DELTA_FORCE%MATMUL                                                                                                            MATMUL #         @                                                     #         @                                                   #NORM%SQRT    #SPANDISP    #SPANDISPLEN    #OUTNORM_DISP    #OUTNORM_ROT                                                    SQRT          
                                                     
    p          5 � p        r        5 � p        r                                
                                                       D @                                   
                 D @                                   
       #         @                                                   #NORM_OLD%SQRT    #SPANDISP     #SPANDISPLEN !   #OUTNORM "                                                   SQRT          
                                                      
    p          5 � p        r !       5 � p        r !                               
                                  !                     D @                              "     
       #         @                                 #                       �   8      fn#fn    �   �       DELTAR    |  @       BETAGENALFA    �  @       GAMMAGENALFA    �  @       ALFA_M    <  @       ALFA_F    |  @       A0    �  @       A1    �  @       A2    <  @       A3    |  @       A4    �  @       A5    �  @       A6    <  @       A7    |  @       A8 -   �  �       NLAMS_SHELLSOLVER_STEP_GALFA @   W  @     NLAMS_SHELLSOLVER_STEP_GALFA%ANSSIZE+NLAMS_INIT 4   �  ?      NLAMS_SHELLSOLVER_STEP_GALFA%MATMUL +   �  H       NLAMS_SHELLSOLVER_STEP_INI )     H       NLAMS_SHELLSOLVER_COEFFS "   f  �       NLAMS_DELTA_FORCE 5   �  @     NLAMS_DELTA_FORCE%ANSSIZE+NLAMS_INIT )   +  ?      NLAMS_DELTA_FORCE%MATMUL    j  H       NLAMS_APPLY_BC    �  �       NORM    K  =      NORM%SQRT    �  �   a   NORM%SPANDISP !   <	  @   a   NORM%SPANDISPLEN "   |	  @   a   NORM%OUTNORM_DISP !   �	  @   a   NORM%OUTNORM_ROT    �	  �       NORM_OLD    �
  =      NORM_OLD%SQRT "   �
  �   a   NORM_OLD%SPANDISP %   t  @   a   NORM_OLD%SPANDISPLEN !   �  @   a   NORM_OLD%OUTNORM %   �  H       NLAMS_DEALLOCATE_VAR 