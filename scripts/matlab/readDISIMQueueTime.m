function [T Dt Dc] = readDISIMQueueTime(filename, color)
    m = load(filename);
    T = m(:,1)/60.0;
    Dt = m(:,2)/60.0;
    Dc = m(:,3);
    md = min(Dt);
    Dt = Dt - md;
    
    if (nargout == 0)
        hold on;
        plot(T,Dt,color);
        ylabel('Average Waiting Time [min]');
        xlabel('Time [min]');
        xlim([0 360]);
    end
end