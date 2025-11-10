*** Settings ***
Library           String
Library           SerialLibrary

*** Variables ***
${com}            COM3
${baud}           115200
${board}          nRF
${valid_time}     000120X        # HHMMSS format with X as the end character
${invalid_time}   123X            # Invalid time (too short)
${error_len}      -1X            # Error for wrong length
${error_zero_input}     000000X            # Error for zero time (000000)
${error_zero}	  -4X
${error_value}    -3X            # Error for invalid value (e.g., hours > 23)
${random_char}    AX 

*** Test Cases ***
Connect Serial
    Log To Console    Connecting to ${board}
    Add Port    ${com}    baudrate=${baud}    encoding=ascii
    Port Should Be Open    ${com}
    Reset Input Buffer
    Reset Output Buffer

Serial Led Control with Valid Time
    # Send valid time sequence (e.g., 000120X)
    Write Data   ${valid_time}   encoding=ascii
    Log To Console   Send sequence ${valid_time}

    # Receive data until terminator X (ASCII 58)
    ${read} =   Read Until   terminator=58   encoding=ascii 

    Log To Console   Received ${read}
    
    Should Be Equal As Strings   ${read}    80X
    Log To Console   Tested ${read} is same as 80X

Serial Led Control with Invalid Time Length
    Write Data   ${invalid_time}   encoding=ascii
    Log To Console   Send sequence ${invalid_time}

    ${read} =   Read Until   terminator=58   encoding=ascii 

    
    Log To Console   Received ${read}
    
    Should Be Equal As Strings   ${read}    ${error_len}
    Log To Console   Tested ${read} is same as ${error_len}

Serial Led Control with Zero Time
    Write Data   ${error_zero_input}   encoding=ascii
    Log To Console   Send sequence ${error_zero}

    # Receive data until terminator X (ASCII 58)
    ${read} =   Read Until   terminator=58   encoding=ascii 

    Log To Console   Received ${read}
    
    # Check for expected error (-4) due to zero time
    Should Be Equal As Strings   ${read}    ${error_zero}
    Log To Console   Tested ${read} is same as ${error_zero}

Serial Led Control with Invalid Time Value
    Write Data   256000X   encoding=ascii
    Log To Console   Send sequence 256000X

    # Receive data until terminator X (ASCII 58)
    ${read} =   Read Until   terminator=58   encoding=ascii 

    Log To Console   Received ${read}
    
    Should Be Equal As Strings   ${read}    ${error_value}
    Log To Console   Tested ${read} is same as ${error_value}

Serial Led Control with Invalid Character
    [Documentation]    Test the system's behavior when an invalid character (other than X) is sent.
    Log To Console    Send random character (${random_char}) to the system
    Write Data   ${random_char}   encoding=ascii
    ${read} =   Read Until   terminator=58   encoding=ascii 
    Log To Console   Received ${read}

    Should Be Equal As Strings   ${read}    ${error_len}
    Log To Console   Tested ${read} is same as ${error_len}

Disconnect Serial
    Log To Console    Disconnecting ${board}
    [TearDown]    Delete Port    ${com}
