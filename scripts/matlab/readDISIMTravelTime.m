function [T D Dc] = readDISIMTravelTime(filename, color)
    fp = fopen(filename);
    
    ctime = [];
    travel = [];
    line = fgetl(fp);
    while ((isempty(line)) || (line(1) ~= -1))
        [a,c] = sscanf(line, 'Travel time of car %d: %f seconds (%f)');
        if (c == 3)
            ctime = [ctime a(3)];
            travel = [travel a(2)];
        end
        line = fgetl(fp);
    end
    
    offset = 60;
    t = [];
    D = [];
    Dc = [];
    T = [];
    for i = 1:length(ctime)
        if (ctime(i) < offset)
            t = [t travel(i)];
        else
            T = [T offset/60.0];
            D = [D mean(t)/60.0];
            Dc = [Dc length(t)];
            offset = offset + 60;
            t = [travel(i)];
        end
    end
    
    if (nargout == 0)
        if (nargin == 1), color = 'b'; end
        plot(T,D,color);
        ylabel('Average Travel Time [min]');
        xlabel('Time [min]');
    end
end