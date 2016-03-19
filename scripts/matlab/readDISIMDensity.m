function [T D] = readDISIMDensity(filename, color)
    m = load(filename);
    T = m(:,1)/60.0;
    D = m(:,2);
    
    if (nargout == 0)
        hold on;
        plot(T,D,color);
        ylabel('Average Density [veh/km]');
        xlabel('Time [min]');
        xlim([0 360]);
    end
end