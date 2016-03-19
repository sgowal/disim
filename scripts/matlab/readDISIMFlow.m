function [T D] = readDISIMFlow(filename, color)
    m = load(filename);
    T = m(:,1)/60.0;
    D = m(:,2);
    
    if (nargout == 0)
        hold on;
        plot(T,D,color);
        ylabel('Average Flow [veh/h]');
        xlabel('Time [min]');
        xlim([0 360]);
    end
end