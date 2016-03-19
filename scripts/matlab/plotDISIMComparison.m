function plotDISIMComparison()
    
    close all;

    prefix = {'data/nocontrol','data/alinea','data/queuetimecontrol','data/timecoordinated'};
    colors = {'b', 'r', 'g', 'k'};
    leg = {'No ramp control', 'ALINEA', 'Queue control','Coordinated'};
    
    % prefix = {'data/nocontrol','data/queuecontrol_time_K0.1','data/queuecontrol_time_K0.01','data/queuecontrol_count_K0.1','data/queuecontrol_count_K0.01'};
    % colors = {'b', 'r', 'g', 'k', 'm'};
    % leg = {'No ramp control', 'Q 1', 'Q 2', 'Q 3', 'Q 4'};

    % Travel time
    figure;
    hold on;
    for i = 1:length(prefix)
        [T D Dc] = readDISIMTravelTime(sprintf('%s/log.txt', prefix{i}));
        plot(T,smooth((D.*Dc)/60,'rlowess'),colors{i},'LineWidth',2);
    end
    hold off;
    ylabel('Vehicle Hours Traveled');
    xlabel('Time [min]');
    xlim([0 360]);
    legend(leg);
    
    figure;
    hold on;
    for i = 1:length(prefix)
        [T D Dc] = readDISIMTravelTime(sprintf('%s/log.txt', prefix{i}));
        plot(T,D,colors{i},'LineWidth',2);
    end
    hold off;
    ylabel('Average Travel Time [min]');
    xlabel('Time [min]');
    xlim([0 360]);
    legend(leg);
    
    % Queue time
    figure;
    hold on;
    for i = 1:length(prefix)
        [~, Dt1{i} Dc1{i}] = readDISIMQueueTime(sprintf('%s/rampmeter1.txt', prefix{i}));
        [~, Dt2{i} Dc2{i}] = readDISIMQueueTime(sprintf('%s/rampmeter2.txt', prefix{i}));
        [~, Dt3{i} Dc3{i}] = readDISIMQueueTime(sprintf('%s/rampmeter3.txt', prefix{i}));
        [~, Dt4{i} Dc4{i}] = readDISIMQueueTime(sprintf('%s/rampmeter4.txt', prefix{i}));
        [~, Dt5{i} Dc5{i}] = readDISIMQueueTime(sprintf('%s/rampmeter5.txt', prefix{i}));
        [T Dt6{i} Dc6{i}] = readDISIMQueueTime(sprintf('%s/rampmeter6.txt', prefix{i}));
        D = Dt1{i}.*Dc1{i} + Dt2{i}.*Dc2{i} + Dt3{i}.*Dc3{i} + Dt4{i}.*Dc4{i} + Dt5{i}.*Dc5{i} + Dt6{i}.*Dc6{i};
        D = D./(Dc1{i} + Dc2{i} + Dc3{i} + Dc4{i} + Dc5{i} + Dc6{i});
        plot(T, smooth(D,'rlowess'), colors{i},'LineWidth',2);
    end
    hold off;
    xlabel('Time [min]');
    ylabel('Average Waiting Time [min]');
    xlim([0 360]);
    legend(leg);
    
    figure;
    hold on;
    for i = 1:length(prefix)
        plot(T,max([Dt1{i} Dt2{i} Dt3{i} Dt4{i} Dt5{i} Dt6{i}],[],2),colors{i},'LineWidth',2);
    end
    hold off;
    xlabel('Time [min]');
    ylabel('Maximal Waiting Time [min]');
    xlim([0 360]);
    legend(leg);
end