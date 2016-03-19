function plotPEMSData()
    close all;
    
    readPEMSData('PEMSData/Myrtle_OnRamp_flow.txt','b', true);
    grid on;
    axis([0 1435 0 2000]);
    legend('Myrtle OnRamp');

    readPEMSData('PEMSData/Myrtle_flow.txt','b', true);
    grid on;
    axis([0 1435 0 8000]);
    legend('Myrtle');
    
    readPEMSData('PEMSData/Myrtle_speed.txt','b', true);
    grid on;
    axis([0 1435 0 130]);
    legend('Myrtle');
end