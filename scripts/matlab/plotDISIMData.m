function [] = plotDISIMData(prefix)
    close all;
    
    if (nargin == 0)
        prefix = '../../logs';
    end

    % Average Travel Time
    figure;
    [T D Dc] = readDISIMTravelTime(sprintf('%s/log.txt', prefix),'b');
    plot(T,smooth((D.*Dc)/60,'rlowess'),'b');
    ylabel('Vehicle Hours Traveled');
    xlabel('Time [min]');
    xlim([0 360]);
    
    figure;
    plot(T,D,'b');
    ylabel('Average Travel Time [min]');
    xlabel('Time [min]');
    xlim([0 360]);
    
    fprintf(1,'Total number of vehicle served during 6 hours: %d\n', sum(Dc));
    
    % Average Queue Time
    figure;
    [~, Dt1 Dc1] = readDISIMQueueTime(sprintf('%s/rampmeter1.txt', prefix));
    [~, Dt2 Dc2] = readDISIMQueueTime(sprintf('%s/rampmeter2.txt', prefix));
    [~, Dt3 Dc3] = readDISIMQueueTime(sprintf('%s/rampmeter3.txt', prefix));
    [~, Dt4 Dc4] = readDISIMQueueTime(sprintf('%s/rampmeter4.txt', prefix));
    [~, Dt5 Dc5] = readDISIMQueueTime(sprintf('%s/rampmeter5.txt', prefix));
    [T Dt6 Dc6] = readDISIMQueueTime(sprintf('%s/rampmeter6.txt', prefix));
    D = Dt1.*Dc1 + Dt2.*Dc2 + Dt3.*Dc3 + Dt4.*Dc4 + Dt5.*Dc5 + Dt6.*Dc6;
    D = D./(Dc1 + Dc2 + Dc3 + Dc4 + Dc5 + Dc6);
    plot(T, smooth(D,'rlowess'), 'b');
    xlabel('Time [min]');
    ylabel('Average Waiting Time [min]');
    xlim([0 360]);
    
    figure;
    readDISIMQueueTime(sprintf('%s/rampmeter1.txt', prefix),'b');
    readDISIMQueueTime(sprintf('%s/rampmeter2.txt', prefix),'r');
    readDISIMQueueTime(sprintf('%s/rampmeter3.txt', prefix),'g');
    readDISIMQueueTime(sprintf('%s/rampmeter4.txt', prefix),'m');
    readDISIMQueueTime(sprintf('%s/rampmeter5.txt', prefix),'c');
    readDISIMQueueTime(sprintf('%s/rampmeter6.txt', prefix),'k');
    legend('1','2','3','4','5','6');
    
    % Fundamental Diagram
    figure;
    [~, Df1] = readDISIMFlow(sprintf('%s/huntington_flow_1.txt', prefix));
    [~, Df2] = readDISIMFlow(sprintf('%s/huntington_flow_2.txt', prefix));
    [~, Df3] = readDISIMFlow(sprintf('%s/huntington_flow_3.txt', prefix));
    [~, Df4] = readDISIMFlow(sprintf('%s/huntington_flow_4.txt', prefix));
    Df = (Df1 + Df2 + Df3 + Df4)./4;
    [~, Dk1] = readDISIMDensity(sprintf('%s/huntington_density_1.txt', prefix));
    [~, Dk2] = readDISIMDensity(sprintf('%s/huntington_density_2.txt', prefix));
    [~, Dk3] = readDISIMDensity(sprintf('%s/huntington_density_3.txt', prefix));
    [~, Dk4] = readDISIMDensity(sprintf('%s/huntington_density_4.txt', prefix));
    Dk = (Dk1 + Dk2 + Dk3 + Dk4)./4;
    hold on;
    plot([0 30 128], [0 1800 0], 'k', 'LineWidth', 2);
    plot(Dk, Df, 'o', 'MarkerEdgeColor','k', 'MarkerFaceColor',[0.5 0.5 0.5], 'MarkerSize', 5);
    plot(Dk1, Df1, 'o', 'MarkerEdgeColor','k', 'MarkerFaceColor',[1.0 0.0 0.0], 'MarkerSize', 5);
    plot(Dk2, Df2, 'o', 'MarkerEdgeColor','k', 'MarkerFaceColor',[0.0 1.0 0.0], 'MarkerSize', 5);
    plot(Dk3, Df3, 'o', 'MarkerEdgeColor','k', 'MarkerFaceColor',[0.0 0.0 1.0], 'MarkerSize', 5);
    plot(Dk4, Df4, 'o', 'MarkerEdgeColor','k', 'MarkerFaceColor',[1.0 1.0 0.0], 'MarkerSize', 5);
    xlabel('Average Density [veh/km/lane]');
    ylabel('Average Flow [veh/h/lane]');
    legend('CTMSIM diagram', 'Average', 'Lane 1 (Left)', 'Lane 2', 'Lane 3', 'Lane 4 (Right)');
    hold off;
    
    figure;
    hold on;
    plot([0 30 128], [0 1800 0], 'k', 'LineWidth', 2);
    plot(Dk, Df, 'o', 'MarkerEdgeColor','k', 'MarkerFaceColor',[0.5 0.5 0.5], 'MarkerSize', 5);
    xlabel('Average Density [veh/km/lane]');
    ylabel('Average Flow [veh/h/lane]');
    legend('CTMSIM diagram', 'Average');
    hold off;
end