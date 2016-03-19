function plotDISIMPEMSFundamental()
    prefix = 'data/nocontrol';

    [~,D1 D2] = readPEMSDualData('PEMSData/Huntington_Lane1_fundamental.txt');
    [~, Df1] = readDISIMFlow(sprintf('%s/huntington_flow_1.txt', prefix));
    [~, Dk1] = readDISIMDensity(sprintf('%s/huntington_density_1.txt', prefix));
    
    figure;
    hold on;
    plot(D1*167,D2*12,'o', 'MarkerEdgeColor','k', 'MarkerFaceColor','b', 'MarkerSize', 5); % 167 = 1/g-factor in km (PEMS g-factor ~ 19.6 feet)
    plot(Dk1, Df1, 'o', 'MarkerEdgeColor','k', 'MarkerFaceColor','r', 'MarkerSize', 5);
    hold off;
    xlabel('Average Density [veh/km/lane]');
    ylabel('Average Flow [veh/h/lane]');
    legend('PEMS Lane 1', 'DISIM Lane 1');
end