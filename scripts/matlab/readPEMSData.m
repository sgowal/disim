function [T D] = readPEMSData(filename, color, plotme)
    % Get last part of the filename
    last_index = -1;
    for i = length(filename):-1:1
        if (filename(i) == '_')
            last_index = i;
            break;
        end
    end
    if (last_index == -1)
        fprintf(2, 'Wrong filename format.\n');
    end
    type = filename(last_index+1:end-4);
    fprintf(1,'Type of file: %s\n', type);
    Z = 0;
    if (strcmp(type,'flow'))
        Z = 0;
    elseif (strcmp(type,'occupancy'))
        Z = 1;
    elseif (strcmp(type,'speed'))
        Z = 2;
    elseif (strcmp(type,'truck'))
        Z = 3;
    else
        fprintf(2,'Unknown type: %s\n', type);
    end
    
    % Read file
    A = importdata(filename, '\t', 1);
    D = A.data(:,end-2);
    T = zeros(size(D,1),1);
    for i = 2:size(A.textdata,1)
        timestr = A.textdata{i,1};
        B = sscanf(timestr,'%d/%d/%d %d:%d');
        T(i-1) = B(5) + B(4)*60;
    end
    for i = 1:length(D)
        if (A.data(i,end) == 0 && i ~= 1)
            D(i) = D(i-1);
        end
    end
    
    % Plot
    if (nargin == 3 && plotme == false)
        return;
    end
    figure();
    if (nargin == 2)
        figure(Z+1);
        hold on;
    else
        color = 'b';
    end
    xlabel('Time [min]');
    switch(Z)
        case 0
            plot(T,D*12,color,'LineWidth',2);
            ylabel('Flow [veh/h]');
        case 1
            plot(T,D*100,color,'LineWidth',2);
            ylabel('Occupancy [%]');
        case 2
            plot(T,D*1.609344,color,'LineWidth',2);
            ylabel('Speed [km/h]');
        case 3
            plot(T,D,color,'LineWidth',2);
            ylabel('Proportion of trucks [%]');
        otherwise
            plot(T,D,color,'LineWidth',2);
    end
    if (nargin == 2)
        hold off;
    end
end